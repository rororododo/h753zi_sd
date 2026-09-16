/**
  ******************************************************************************
  * @file    sd_diskio.c
  * @brief   FatFs SDMMC diskio implementation
  ******************************************************************************
  */

#include "sd_diskio.h"
#include "ff.h"
#include <stdio.h>

extern SD_HandleTypeDef hsd1;
static volatile DSTATUS Stat = STA_NOINIT;

/* Wait for SD card ready */
static uint32_t SD_WaitReady(uint32_t timeout_ms)
{
  uint32_t tickstart = HAL_GetTick();
  while (HAL_SD_GetCardState(&hsd1) != HAL_SD_CARD_TRANSFER)
  {
    if ((HAL_GetTick() - tickstart) >= timeout_ms)
    {
      return 1; /* Timeout */
    }
  }
  return 0; /* OK */
}

DSTATUS disk_initialize(BYTE pdrv)
{
  if (pdrv != 0) return STA_NOINIT;

  if (hsd1.State == HAL_SD_STATE_READY)
  {
    Stat &= ~STA_NOINIT;
    printf("    [diskio] disk_initialize: OK\r\n");
  }
  else
  {
    Stat = STA_NOINIT;
    printf("    [diskio] disk_initialize: STA_NOINIT (State=0x%02X)\r\n", (unsigned int)hsd1.State);
  }
  return Stat;
}

DSTATUS disk_status(BYTE pdrv)
{
  if (pdrv != 0) return STA_NOINIT;
  return Stat;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  int retry = 3;

  if (pdrv != 0 || !count) return RES_PARERR;
  if (Stat & STA_NOINIT)
  {
    printf("    [diskio] disk_read error: STA_NOINIT!\r\n");
    return RES_NOTRDY;
  }

  while (retry--)
  {
    if (SD_WaitReady(2000) != 0)
    {
      printf("    [diskio] disk_read error: SD_WaitReady timeout!\r\n");
      return RES_ERROR;
    }

    hsd1.ErrorCode = HAL_SD_ERROR_NONE;
    status = HAL_SD_ReadBlocks(&hsd1, (uint8_t *)buff, (uint32_t)sector, count, 5000);
    if (status == HAL_OK)
    {
      if (SD_WaitReady(2000) == 0)
      {
        return RES_OK;
      }
    }
    HAL_Delay(10);
  }

  printf("    [diskio] disk_read error: HAL status=%d, ErrorCode=0x%08lX (sector %lu, count %u)\r\n",
         status, hsd1.ErrorCode, (unsigned long)sector, count);
  return RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  int retry = 3;

  if (pdrv != 0 || !count) return RES_PARERR;
  if (Stat & STA_NOINIT) return RES_NOTRDY;

  while (retry--)
  {
    if (SD_WaitReady(2000) != 0)
    {
      printf("    [diskio] disk_write error: SD_WaitReady timeout!\r\n");
      return RES_ERROR;
    }

    hsd1.ErrorCode = HAL_SD_ERROR_NONE;
    status = HAL_SD_WriteBlocks(&hsd1, (uint8_t *)buff, (uint32_t)sector, count, 5000);
    if (status == HAL_OK)
    {
      if (SD_WaitReady(2000) == 0)
      {
        return RES_OK;
      }
    }
    HAL_Delay(10);
  }

  printf("    [diskio] disk_write error: HAL status=%d, ErrorCode=0x%08lX (sector %lu, count %u)\r\n",
         status, hsd1.ErrorCode, (unsigned long)sector, count);
  return RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
  DRESULT res = RES_ERROR;
  HAL_SD_CardInfoTypeDef CardInfo;

  if (pdrv != 0) return RES_PARERR;
  if (Stat & STA_NOINIT) return RES_NOTRDY;

  switch (cmd)
  {
  case CTRL_SYNC:
    if (SD_WaitReady(2000) == 0)
    {
      res = RES_OK;
    }
    break;

  case GET_SECTOR_COUNT:
    HAL_SD_GetCardInfo(&hsd1, &CardInfo);
    *(DWORD *)buff = CardInfo.LogBlockNbr;
    res = RES_OK;
    break;

  case GET_SECTOR_SIZE:
    HAL_SD_GetCardInfo(&hsd1, &CardInfo);
    *(WORD *)buff = CardInfo.LogBlockSize;
    res = RES_OK;
    break;

  case GET_BLOCK_SIZE:
    HAL_SD_GetCardInfo(&hsd1, &CardInfo);
    *(DWORD *)buff = CardInfo.LogBlockSize / 512;
    res = RES_OK;
    break;

  default:
    res = RES_PARERR;
    break;
  }

  return res;
}

/* User defined function to give a current time to fatfs module */
DWORD get_fattime(void)
{
  return ((DWORD)(2026 - 1980) << 25) /* Year 2026 */
       | ((DWORD)9 << 21)             /* Month 9 */
       | ((DWORD)14 << 16)            /* Day 14 */
       | ((DWORD)12 << 11)            /* Hour 12 */
       | ((DWORD)0 << 5)              /* Min 0 */
       | ((DWORD)0 >> 1);             /* Sec 0 */
}
