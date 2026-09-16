/**
  ******************************************************************************
  * @file    Inc/main.h
  * @brief   Header for main.c module
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32h7xx_hal.h"
#include <stdio.h>
#include <string.h>

/* Nucleo-H753ZI On-board LEDs */
#define LED1_PIN                GPIO_PIN_0
#define LED1_GPIO_PORT          GPIOB
#define LED2_PIN                GPIO_PIN_1
#define LED2_GPIO_PORT          GPIOE
#define LED3_PIN                GPIO_PIN_14
#define LED3_GPIO_PORT          GPIOB

/* ST-LINK VCP USART3 Pins */
#define VCP_TX_PIN              GPIO_PIN_8
#define VCP_TX_PORT             GPIOD
#define VCP_RX_PIN              GPIO_PIN_9
#define VCP_RX_PORT             GPIOD

/* SDMMC1 MicroSD Pins (D43 ~ D48 on Nucleo-144) */
/* PC8: D0, PC9: D1, PC10: D2, PC11: D3, PC12: CLK, PD2: CMD */
#define SDMMC1_D0_PIN           GPIO_PIN_8
#define SDMMC1_D0_PORT          GPIOC
#define SDMMC1_D1_PIN           GPIO_PIN_9
#define SDMMC1_D1_PORT          GPIOC
#define SDMMC1_D2_PIN           GPIO_PIN_10
#define SDMMC1_D2_PORT          GPIOC
#define SDMMC1_D3_PIN           GPIO_PIN_11
#define SDMMC1_D3_PORT          GPIOC
#define SDMMC1_CLK_PIN          GPIO_PIN_12
#define SDMMC1_CLK_PORT         GPIOC
#define SDMMC1_CMD_PIN          GPIO_PIN_2
#define SDMMC1_CMD_PORT         GPIOD

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
