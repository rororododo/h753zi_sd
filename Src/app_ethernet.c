/**
 ******************************************************************************
 * @file    app_ethernet.c
 * @brief   LwIP Initialization, UDP Server, and MicroSD Packet Storage
 ******************************************************************************
 */

#include "app_ethernet.h"
#include "ethernetif.h"
#include "ff.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
#include "netif/ethernet.h"
#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <string.h>

struct netif gnetif;
static struct udp_pcb *udp_server_pcb;
static uint32_t last_link_check = 0;
static uint32_t packet_count = 0;
char current_log_filename[32] = "LOG_0001.TXT";

/* ==============================================================================
 * [기능] 물리 SD 카드를 직접 f_read로 읽어서 터미널(COM3)에 출력
 * ============================================================================== */
void App_Ethernet_Dump_File(const char *filename) {
  FIL fil;
  FRESULT fr = f_open(&fil, filename, FA_READ);
  if (fr != FR_OK) {
    printf("\r\n[SD READ] File '%s' could not be opened (FatFs code: %d)\r\n",
           filename, fr);
    return;
  }

  printf("\r\n================ [PHYSICAL SD CARD: %s] ================\r\n",
         filename);
  char buf[128];
  UINT br;
  UINT total_bytes = 0;
  while (f_read(&fil, buf, sizeof(buf), &br) == FR_OK && br > 0) {
    printf("%.*s", (int)br, buf);
    total_bytes += br;
  }
  f_close(&fil);
  printf("\r\n================ [END OF FILE (%u bytes read)] ================\r\n\r\n",
         total_bytes);
}

void App_Ethernet_Dump_Previous_File(void) {
  int current_idx = atoi(&current_log_filename[4]);

  if (current_idx > 1) {
    char prev_file[32];
    snprintf(prev_file, sizeof(prev_file), "LOG_%04d.TXT", current_idx - 1);
    printf("\r\n================ [PREVIOUS FILE: %s] ================\r\n",
           prev_file);
    App_Ethernet_Dump_File(prev_file);
  } else {
    printf("\r\n>>> [COMMAND 'p'] No previous log file exists on SD Card (Current is %s)\r\n",
           current_log_filename);
  }
}

void App_Ethernet_Dump_Current_File(void) {
  printf("\r\n================ [CURRENT FILE: %s] ================\r\n",
         current_log_filename);
  App_Ethernet_Dump_File(current_log_filename);
}

/* ==============================================================================
 * [핵심 기능 1] SD 카드 번호 자동 증가 (0001 -> 0002 -> 0003 ...)
 * ============================================================================== */
static void App_Ethernet_Init_LogFile(void) {
  FILINFO fno;
  int idx = 1;
  while (idx <= 9999) {
    snprintf(current_log_filename, sizeof(current_log_filename), "LOG_%04d.TXT",
             idx);
    if (f_stat(current_log_filename, &fno) != FR_OK) {
      /* File does not exist yet -> use this filename */
      break;
    }
    idx++;
  }

  FIL fil;
  FRESULT fr = f_open(&fil, current_log_filename, FA_CREATE_ALWAYS | FA_WRITE);
  if (fr == FR_OK) {
    char header[128];
    snprintf(header, sizeof(header),
             "=== STM32H753ZI Session Log: %s ===\r\n\r\n",
             current_log_filename);
    UINT bw;
    f_write(&fil, header, strlen(header), &bw);
    f_close(&fil);
    printf("    [SD CARD] Created new session file: '%s'\r\n",
           current_log_filename);
  } else {
    printf("    [ERROR] Failed to create log file '%s' (fr=%d)\r\n",
           current_log_filename, fr);
  }
}

/**
 * @brief UDP Packet Receive Callback: Saves received payload to SD Card
 * immediately
 */
static void udp_receive_callback(void *arg, struct udp_pcb *upcb,
                                 struct pbuf *p, const ip_addr_t *addr,
                                 u16_t port) {
  if (p != NULL) {
    FIL fil;
    FRESULT fr;
    UINT bw;

    /* UDP로 'p' 또는 'r' 수신 시에도 지원 */
    if (p->tot_len <= 3) {
      char c = ((char *)p->payload)[0];
      if (c == 'p' || c == 'P') {
        App_Ethernet_Dump_Previous_File();
        pbuf_free(p);
        return;
      } else if (c == 'r' || c == 'R') {
        App_Ethernet_Dump_Current_File();
        pbuf_free(p);
        return;
      }
    }

    packet_count++;

    printf("\r\n[ETH RX #%lu] Packet from %s:%d | Length: %u bytes\r\n",
           (unsigned long)packet_count, ip4addr_ntoa(addr), port, p->tot_len);

    /* Open the current session log file in append mode */
    fr = f_open(&fil, current_log_filename, FA_OPEN_APPEND | FA_WRITE);
    if (fr == FR_OK) {
      struct pbuf *q;
      UINT total_written = 0;

      for (q = p; q != NULL; q = q->next) {
        fr = f_write(&fil, q->payload, q->len, &bw);
        if (fr == FR_OK) {
          total_written += bw;
        }
      }

      /* Auto-append newline for clean text display */
      const char newline[] = "\r\n";
      f_write(&fil, newline, 2, &bw);
      f_close(&fil);

      printf("    [SD CARD] Appended %u bytes to '%s'!\r\n", total_written,
             current_log_filename);

      /* Print preview of data */
      printf("    Message Preview: \"%.*s\"\r\n",
             (p->len < 64) ? (int)p->len : 64, (char *)p->payload);
    } else {
      printf("    [ERROR] Failed to open '%s' on SD Card! (fr=%d)\r\n",
             current_log_filename, fr);
    }

    /* Free packet buffer */
    pbuf_free(p);
  }
}

/**
 * @brief Initialize LwIP Network Stack with Static IP (192.168.0.50)
 */
void LwIP_Init(void) {
  ip4_addr_t ipaddr;
  ip4_addr_t netmask;
  ip4_addr_t gw;

  printf("[Step 6] Initializing LwIP Stack (No-OS, Static Allocation)...\r\n");

  /* Initialize LwIP core */
  lwip_init();

  /* Set Static IP Address: 192.168.0.50 */
  IP4_ADDR(&ipaddr, 192, 168, 0, 50);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 192, 168, 0, 1);

  /* Add network interface */
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init,
            &ethernet_input);

  /* Register as default network interface */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif)) {
    netif_set_up(&gnetif);
  } else {
    netif_set_down(&gnetif);
  }

  printf("    -> Static IP Assigned: 192.168.0.50 (Netmask: 255.255.255.0, GW: "
         "192.168.0.1)\r\n");

  /* Start UDP Server on port 8080 */
  udp_server_pcb = udp_new();
  if (udp_server_pcb != NULL) {
    err_t err = udp_bind(udp_server_pcb, IP_ADDR_ANY, 8080);
    if (err == ERR_OK) {
      udp_recv(udp_server_pcb, udp_receive_callback, NULL);
      printf("    -> UDP Receiver listening on Port 8080 (Ready for "
             "packets!)\r\n");
    } else {
      printf("    [ERROR] udp_bind failed (err=%d)!\r\n", err);
    }
  } else {
    printf("    [ERROR] udp_new failed!\r\n");
  }

  /* Create new unique session log file on SD Card */
  App_Ethernet_Init_LogFile();
}

/**
 * @brief Periodic polling function called from main while(1) loop
 */
void LwIP_Process(void) {
  /* Read and process incoming Ethernet packets */
  ethernetif_input(&gnetif);

  /* Handle LwIP timers */
  sys_check_timeouts();

  /* Periodically check cable link state (every 500ms) */
  if ((HAL_GetTick() - last_link_check) >= 500) {
    last_link_check = HAL_GetTick();
    ethernet_link_check_state(&gnetif);
  }
}
