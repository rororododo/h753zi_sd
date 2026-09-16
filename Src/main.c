/**
 ******************************************************************************
 * @file    Src/main.c
 * @brief   STM32H753ZI MicroSD (SDMMC1 4-bit) & VCP (COM3) Test
 ******************************************************************************
 */

#include "main.h"
#include "app_ethernet.h"
#include "ff.h"
#include "sd_diskio.h"


/* Handles */
UART_HandleTypeDef huart3;
SD_HandleTypeDef hsd1;

/* FatFs Variables */
FATFS fs;
FIL fil;
FRESULT fr;

/* Prototypes */
static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MPU_Config(void);

/* Retarget printf to USART3 (ST-LINK VCP) */
int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, 0xFFFF);
  return ch;
}

int _write(int file, char *ptr, int len) {
  HAL_UART_Transmit(&huart3, (uint8_t *)ptr, len, 0xFFFF);
  return len;
}

int main(void) {
  /* MPU Configuration */
  MPU_Config();

  /* STM32H7xx HAL library initialization */
  HAL_Init();

  /* Configure the system clock to 400 MHz */
  SystemClock_Config();

  /* Initialize Peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();

  /* Welcome Banner */
  printf("\r\n\r\n");
  printf("====================================================\r\n");
  printf("  STM32H753ZI Nucleo-144 SD Card Test Application   \r\n");
  printf("  Bus Mode: SDMMC1 4-Bit Wide Bus (D43~D48)         \r\n");
  printf("  VCP BaudRate: 115200 8N1                          \r\n");
  printf("====================================================\r\n\r\n");

  /* Turn on Yellow LED during initialization */
  HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_SET);

  printf("[1] Initializing SDMMC1 Peripheral...\r\n");
  MX_SDMMC1_SD_Init();

  /* Initialize SD Card via HAL */
  if (HAL_SD_Init(&hsd1) != HAL_OK) {
    printf("[ERROR] HAL_SD_Init failed! ErrorCode: 0x%08lX\r\n",
           hsd1.ErrorCode);
    printf("Please check SD Card jumper wiring (D43~D48) and 3V3/GND!\r\n");
    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_SET); /* Red LED */
    Error_Handler();
  }
  printf("    -> HAL_SD_Init: OK!\r\n");

  /* Query Card Info */
  HAL_SD_CardInfoTypeDef cardInfo;
  HAL_SD_GetCardInfo(&hsd1, &cardInfo);
  uint32_t capacity_mb = (uint32_t)((uint64_t)cardInfo.BlockNbr *
                                    cardInfo.BlockSize / (1024 * 1024));
  printf("    -> Card Type: %s, Capacity: %lu MB\r\n",
         (cardInfo.CardType == CARD_SDHC_SDXC) ? "SDHC/SDXC" : "Standard SD",
         capacity_mb);

  /* Use 1-bit Bus Mode for maximum jumper wire reliability */
  if (HAL_SD_ConfigWideBusOperation(&hsd1, SDMMC_BUS_WIDE_1B) != HAL_OK) {
    printf("    [WARN] Failed to configure 1-Bit bus mode!\r\n");
  } else {
    printf("    -> 1-Bit Bus Mode: Enabled!\r\n");
  }

  /* Mount FatFs */
  printf("[2] Mounting FAT File System...\r\n");
  fr = f_mount(&fs, "", 1);
  if (fr != FR_OK) {
    printf("[ERROR] f_mount failed! FatFs Result Code: %d\r\n", fr);
    HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_SET); /* Red LED */
    Error_Handler();
  }
  printf("    -> File System Mounted: OK!\r\n");

  /* Ponytail: Turn off Yellow (Init), Turn on Green (Ready) directly */
  HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED1_GPIO_PORT, LED1_PIN, GPIO_PIN_SET);

  /* Initialize Ethernet & LwIP Stack (Static IP: 192.168.0.50, Port 8080) */
  LwIP_Init();

  printf("\r\n====================================================\r\n");
  printf("  Ready for UDP Packets on 192.168.0.50:8080 !       \r\n");
  printf("  Received packets will be appended to '%s'\r\n",
         current_log_filename);
  printf("====================================================\r\n\r\n");

  /* Main Loop: Process Ethernet Packets & Heartbeat */
  uint32_t count = 0;
  uint32_t last_heartbeat = HAL_GetTick();

  while (1) {
    LwIP_Process();

    /* Clear any UART line errors (Overrun, Framing, Noise) so receiver never halts */
    huart3.Instance->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF;

    /* Check for incoming command on COM3 ('p' for previous file, 'r' for current file) */
    if (__HAL_UART_GET_FLAG(&huart3, UART_FLAG_RXNE)) {
      uint8_t ch = (uint8_t)(huart3.Instance->RDR & 0xFF);
      if (ch == 'p' || ch == 'P') {
        App_Ethernet_Dump_Previous_File();
      } else if (ch == 'r' || ch == 'R') {
        App_Ethernet_Dump_Current_File();
      }
    }

    if ((HAL_GetTick() - last_heartbeat) >= 1000) {
      last_heartbeat = HAL_GetTick();
      HAL_GPIO_TogglePin(LED1_GPIO_PORT, LED1_PIN);
      printf("Status: System Running... (Heartbeat %lu)\r\n", ++count);
    }
  }
}

/**
 * @brief System Clock Configuration (400MHz using HSI)
 */
static void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /* Supply configuration update enable */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /* Configure the main internal regulator output voltage */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  uint32_t t0 = HAL_GetTick();
  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    if ((HAL_GetTick() - t0) > 50)
      break;
  }

  /* Enable HSI Oscillator and activate PLL with HSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 50; /* 64 / 4 * 50 = 800 MHz VCO */
  RCC_OscInitStruct.PLL.PLLP = 2;  /* 800 / 2 = 400 MHz SysClk */
  RCC_OscInitStruct.PLL.PLLQ = 4;  /* 800 / 4 = 200 MHz PLL1Q for SDMMC */
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Select PLL as system clock source and configure the HCLK, PCLK1, PCLK2,
   * PCLK3, PCLK4 clocks dividers */
  RCC_ClkInitStruct.ClockType =
      (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 |
       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2; /* 200 MHz */
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);

  /* Configure SDMMC1 and USART3 clock sources */
  PeriphClkInitStruct.PeriphClockSelection =
      RCC_PERIPHCLK_SDMMC | RCC_PERIPHCLK_USART3;
  PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
  PeriphClkInitStruct.Usart234578ClockSelection =
      RCC_USART234578CLKSOURCE_D2PCLK1;
  HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

/**
 * @brief SDMMC1 Initialization
 */
static void MX_SDMMC1_SD_Init(void) {
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide =
      SDMMC_BUS_WIDE_1B; /* Initially 1-bit during identification */
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv =
      16; /* Robust clock divisor for jumper wires (~6.25MHz) */
}

/**
 * @brief USART3 Initialization (ST-LINK VCP)
 */
static void MX_USART3_UART_Init(void) {
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart3);
}

/**
 * @brief GPIO Initialization (LEDs)
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  /* Configure PB0 (Green LED1) and PB14 (Red LED3) */
  GPIO_InitStruct.Pin = LED1_PIN | LED3_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Configure PE1 (Yellow LED2) */
  GPIO_InitStruct.Pin = LED2_PIN;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /* All LEDs off initially */
  HAL_GPIO_WritePin(GPIOB, LED1_PIN | LED3_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOE, LED2_PIN, GPIO_PIN_RESET);
}

/**
 * @brief MPU Configuration to disable D-Cache hazards during SD test
 */
static void MPU_Config(void) {
  /* Disable MPU and D-Cache for simple, rock-solid DMA/buffer coherency */
  HAL_MPU_Disable();
  SCB_DisableDCache();
  SCB_EnableICache();
}

void Error_Handler(void) {
  __disable_irq();
  while (1) {
    HAL_GPIO_TogglePin(LED3_GPIO_PORT, LED3_PIN);
    for (volatile int i = 0; i < 1000000; i++) {
    }
  }
}
