/**
 ******************************************************************************
 * @file    ethernetif.c
 * @brief   Bare-Metal Ethernet interface driver for STM32H753ZI & LAN8742 PHY
 ******************************************************************************
 */

#include "ethernetif.h"
#include "lan8742.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/opt.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <string.h>

#define ETH_RX_BUFFER_SIZE 1536
#define ETH_DMA_TRANSMIT_TIMEOUT 100

/* Network Interface name */
#define IFNAME0 's'
#define IFNAME1 't'

/* ==============================================================================
 * [핵심 기능 2-1] 수신 패킷을 RAM_D2 전용 구역(.RxArraySection)에 지정
 * ==============================================================================
 * 1. Rx_Buff는 노트북에서 랜선을 타고 날아오는 패킷 데이터를 직접 받는
 * 버퍼입니다.
 * 2. 일반 전역변수처럼 RAM_D1에 두지 않고, __attribute__((section(...))) 문법을
 * 써서 .RxArraySection 이라는 특별한 꼬리표를 붙여둡니다.
 * 3. 이 꼬리표는 링커 스크립트(FLASH.ld)에 의해 0x30000000(RAM_D2) 주소로 강제
 * 배정됩니다.
 * 4. 결과: 이더넷 DMA 하드웨어가 CPU 간섭 없이 0x30000000 정적 주소로 패킷을
 * 직통 배달합니다.
 * ==============================================================================
 */
ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT]
    __attribute__((section(".RxDecripSection")));
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT]
    __attribute__((section(".TxDecripSection")));
uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_RX_BUFFER_SIZE]
    __attribute__((section(".RxArraySection")));

// 패킷들어오면 주소에 넣어라: 142

ETH_HandleTypeDef EthHandle;
ETH_TxPacketConfigTypeDef TxConfig;
lan8742_Object_t LAN8742;

static uint32_t current_rx_buf_idx = 0;

/* sys_now provides millisecond ticks to LwIP */
u32_t sys_now(void) { return HAL_GetTick(); }

/* Forward declarations */
static void low_level_init(struct netif *netif);
static err_t low_level_output(struct netif *netif, struct pbuf *p);
static struct pbuf *low_level_input(struct netif *netif);

/* PHY IO Callbacks */
static int32_t ETH_PHY_IO_Init(void) {
  HAL_ETH_SetMDIOClockRange(&EthHandle);
  return 0;
}

static int32_t ETH_PHY_IO_DeInit(void) { return 0; }

static int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr,
                                  uint32_t *pRegVal) {
  if (HAL_ETH_ReadPHYRegister(&EthHandle, DevAddr, RegAddr, pRegVal) !=
      HAL_OK) {
    return -1;
  }
  return 0;
}

static int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr,
                                   uint32_t RegVal) {
  if (HAL_ETH_WritePHYRegister(&EthHandle, DevAddr, RegAddr, RegVal) !=
      HAL_OK) {
    return -1;
  }
  return 0;
}

static int32_t ETH_PHY_IO_GetTick(void) { return (int32_t)HAL_GetTick(); }

static lan8742_IOCtx_t LAN8742_IOCtx = {ETH_PHY_IO_Init, ETH_PHY_IO_DeInit,
                                        ETH_PHY_IO_WriteReg, ETH_PHY_IO_ReadReg,
                                        ETH_PHY_IO_GetTick};

/**
 * @brief HAL ETH Rx Allocate Callback
 */
void HAL_ETH_RxAllocateCallback(uint8_t **buff) {
  *buff = Rx_Buff[current_rx_buf_idx];
  current_rx_buf_idx = (current_rx_buf_idx + 1) % ETH_RX_DESC_CNT;
}

/**
 * @brief HAL ETH Rx Link Callback: links received packet buffer to pbuf
 */
void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff,
                            uint16_t Length) {
  struct pbuf **ppStart = (struct pbuf **)pStart;
  struct pbuf **ppEnd = (struct pbuf **)pEnd;
  struct pbuf *p = pbuf_alloc(PBUF_RAW, Length, PBUF_POOL);

  if (p != NULL) {
    pbuf_take(p, buff, Length);
    *ppStart = p;
    *ppEnd = p;
  }
}

void HAL_ETH_TxFreeCallback(uint32_t *buff) {
  /* Nothing needed for blocking transmit */
}

/**
 * @brief Low level hardware initialization
 */
static void low_level_init(struct netif *netif) {
  uint8_t macaddress[6] = {0x00, 0x80, 0xE1, 0x75, 0x30, 0x01}; /* ST MAC */

  EthHandle.Instance = ETH;
  EthHandle.Init.MACAddr = macaddress;
  EthHandle.Init.MediaInterface = HAL_ETH_RMII_MODE;
  EthHandle.Init.RxDesc = DMARxDscrTab;
  EthHandle.Init.TxDesc = DMATxDscrTab;
  EthHandle.Init.RxBuffLen = ETH_RX_BUFFER_SIZE;

  /* Configure Ethernet peripheral (GPIOs, Clocks, MAC, DMA) */
  if (HAL_ETH_Init(&EthHandle) != HAL_OK) {
    printf("[ETH ERROR] HAL_ETH_Init failed!\r\n");
    return;
  }

  /* Set netif hardware address */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;
  memcpy(netif->hwaddr, macaddress, 6);
  netif->mtu = 1500;
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

  /* Set Tx packet config */
  memset(&TxConfig, 0, sizeof(ETH_TxPacketConfigTypeDef));
  TxConfig.Attributes =
      ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;

  /* Initialize LAN8742 PHY */
  LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);
  LAN8742_Init(&LAN8742);

  /* Start ETH peripheral */
  HAL_ETH_Start(&EthHandle);

  printf("    -> Ethernet Hardware & LAN8742 PHY Initialized!\r\n");
}

/**
 * @brief Low level transmit function
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p) {
  uint32_t i = 0;
  uint32_t framelen = 0;
  struct pbuf *q;
  ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT];

  memset(Txbuffer, 0, sizeof(Txbuffer));

  for (q = p; q != NULL; q = q->next) {
    if (i >= ETH_TX_DESC_CNT) {
      return ERR_IF;
    }
    Txbuffer[i].buffer = q->payload;
    Txbuffer[i].len = q->len;
    framelen += q->len;
    if (i > 0) {
      Txbuffer[i - 1].next = &Txbuffer[i];
    }
    if (q->next == NULL) {
      Txbuffer[i].next = NULL;
    }
    i++;
  }

  TxConfig.Length = framelen;
  TxConfig.TxBuffer = Txbuffer;

  if (HAL_ETH_Transmit(&EthHandle, &TxConfig, ETH_DMA_TRANSMIT_TIMEOUT) !=
      HAL_OK) {
    return ERR_TIMEOUT;
  }

  return ERR_OK;
}

/**
 * @brief Low level receive function
 */
static struct pbuf *low_level_input(struct netif *netif) {
  struct pbuf *p = NULL;

  if (HAL_ETH_ReadData(&EthHandle, (void **)&p) == HAL_OK) {
    return p;
  }

  return NULL;
}

/**
 * @brief Initialize netif callbacks
 */
err_t ethernetif_init(struct netif *netif) {
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  low_level_init(netif);
  return ERR_OK;
}

/**
 * @brief Call this periodically from the main loop to process received packets
 */
void ethernetif_input(struct netif *netif) {
  struct pbuf *p;

  do {
    p = low_level_input(netif);
    if (p != NULL) {
      if (netif->input(p, netif) != ERR_OK) {
        pbuf_free(p);
      }
    }
  } while (p != NULL);
}

/**
 * @brief Check PHY link status periodically
 */
void ethernet_link_check_state(struct netif *netif) {
  int32_t linkstate = LAN8742_GetLinkState(&LAN8742);
  if (linkstate <= LAN8742_STATUS_LINK_DOWN) {
    if (netif_is_link_up(netif)) {
      netif_set_link_down(netif);
      printf("[ETH] Link is DOWN (Cable Disconnected)\r\n");
    }
  } else {
    if (!netif_is_link_up(netif)) {
      netif_set_link_up(netif);
      printf("[ETH] Link is UP (Connected at 100Mbps)!\r\n");
    }
  }
}
