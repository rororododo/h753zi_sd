/**
  ******************************************************************************
  * @file    stm32h7xx_hal_msp.c
  * @brief   HAL MSP module for Nucleo-H753ZI (SDMMC1 & USART3 VCP)
  ******************************************************************************
  */

#include "main.h"

void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
}

/**
  * @brief UART MSP Initialization
  *        USART3 is connected to ST-LINK Virtual COM Port:
  *        PD8 -> USART3_TX
  *        PD9 -> USART3_RX
  */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (huart->Instance == USART3)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* PD8 -> USART3_TX, PD9 -> USART3_RX */
    GPIO_InitStruct.Pin = VCP_TX_PIN | VCP_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* huart)
{
  if (huart->Instance == USART3)
  {
    __HAL_RCC_USART3_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOD, VCP_TX_PIN | VCP_RX_PIN);
  }
}

/**
  * @brief SD MSP Initialization
  *        SDMMC1 Pins mapped on Nucleo-144 CN8 (D43 ~ D48):
  *        PC8  -> SDMMC1_D0  (D43)
  *        PC9  -> SDMMC1_D1  (D44)
  *        PC10 -> SDMMC1_D2  (D45)
  *        PC11 -> SDMMC1_D3  (D46)
  *        PC12 -> SDMMC1_CK  (D47)
  *        PD2  -> SDMMC1_CMD (D48)
  */
void HAL_SD_MspInit(SD_HandleTypeDef* hsd)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  if (hsd->Instance == SDMMC1)
  {
    /* Enable SDMMC1 Clock */
    __HAL_RCC_SDMMC1_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* Configure PC8, PC9, PC10, PC11 (D0..D3) with Pull-Up */
    GPIO_InitStruct.Pin = SDMMC1_D0_PIN | SDMMC1_D1_PIN | SDMMC1_D2_PIN | SDMMC1_D3_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Configure PC12 (Clock) with No-Pull */
    GPIO_InitStruct.Pin = SDMMC1_CLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Configure PD2 (CMD) with Pull-Up */
    GPIO_InitStruct.Pin = SDMMC1_CMD_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDIO1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  }
}

void HAL_SD_MspDeInit(SD_HandleTypeDef* hsd)
{
  if (hsd->Instance == SDMMC1)
  {
    __HAL_RCC_SDMMC1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOC, SDMMC1_D0_PIN | SDMMC1_D1_PIN | SDMMC1_D2_PIN | SDMMC1_D3_PIN | SDMMC1_CLK_PIN);
    HAL_GPIO_DeInit(GPIOD, SDMMC1_CMD_PIN);
  }
}

/**
  * @brief Ethernet MSP Initialization
  *        RMII Pins mapped on Nucleo-144:
  *        PA1  -> RMII_REF_CLK
  *        PA2  -> RMII_MDIO
  *        PA7  -> RMII_CRS_DV
  *        PC1  -> RMII_MDC
  *        PC4  -> RMII_RXD0
  *        PC5  -> RMII_RXD1
  *        PG11 -> RMII_TX_EN
  *        PG13 -> RMII_TXD0
  *        PB13 / PG12 -> RMII_TXD1
  */
void HAL_ETH_MspInit(ETH_HandleTypeDef* heth)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if (heth->Instance == ETH)
  {
    /* Enable GPIOs Clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /* Enable D2 SRAM Clocks for Ethernet DMA descriptors & buffers */
    __HAL_RCC_D2SRAM1_CLK_ENABLE();
    __HAL_RCC_D2SRAM2_CLK_ENABLE();
    __HAL_RCC_D2SRAM3_CLK_ENABLE();

    /* Enable Ethernet Clocks */
    __HAL_RCC_ETH1MAC_CLK_ENABLE();
    __HAL_RCC_ETH1TX_CLK_ENABLE();
    __HAL_RCC_ETH1RX_CLK_ENABLE();

    /* Configure PA1 (REF_CLK), PA2 (MDIO), PA7 (CRS_DV) */
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure PC1 (MDC), PC4 (RXD0), PC5 (RXD1) */
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* Configure PG11 (TX_EN), PG13 (TXD0), PG12 (TXD1) */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    /* Configure PB13 (TXD1 - Nucleo-144 default jumper) */
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Enable the Ethernet global Interrupt */
    HAL_NVIC_SetPriority(ETH_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(ETH_IRQn);
  }
}

void HAL_ETH_MspDeInit(ETH_HandleTypeDef* heth)
{
  if (heth->Instance == ETH)
  {
    __HAL_RCC_ETH1MAC_CLK_DISABLE();
    __HAL_RCC_ETH1TX_CLK_DISABLE();
    __HAL_RCC_ETH1RX_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7);
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5);
    HAL_GPIO_DeInit(GPIOG, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13);

    HAL_NVIC_DisableIRQ(ETH_IRQn);
  }
}
