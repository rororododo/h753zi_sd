/**
 ******************************************************************************
 * @file    app_ethernet.h
 * @brief   Ethernet Application & SD Card Packet Storage Header
 ******************************************************************************
 */

#ifndef __APP_ETHERNET_H__
#define __APP_ETHERNET_H__

#include "lwip/netif.h"

extern char current_log_filename[32];

void LwIP_Init(void);
void LwIP_Process(void);
void App_Ethernet_Dump_File(const char *filename);
void App_Ethernet_Dump_Previous_File(void);
void App_Ethernet_Dump_Current_File(void);

#endif /* __APP_ETHERNET_H__ */
