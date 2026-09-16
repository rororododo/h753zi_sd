/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  */

#include "main.h"
#include "stm32h7xx_it.h"

void NMI_Handler(void)
{
  while (1) {}
}

void HardFault_Handler(void)
{
  /* Light up RED LED on hardfault */
  HAL_GPIO_WritePin(LED3_GPIO_PORT, LED3_PIN, GPIO_PIN_SET);
  while (1) {}
}

void MemManage_Handler(void)
{
  while (1) {}
}

void BusFault_Handler(void)
{
  while (1) {}
}

void UsageFault_Handler(void)
{
  while (1) {}
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
  HAL_IncTick();
}

extern ETH_HandleTypeDef EthHandle;

void ETH_IRQHandler(void)
{
  HAL_ETH_IRQHandler(&EthHandle);
}
