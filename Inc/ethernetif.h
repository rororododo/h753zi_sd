/**
  ******************************************************************************
  * @file    ethernetif.h
  * @brief   Ethernet interface header for STM32H753ZI and LwIP
  ******************************************************************************
  */

#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/err.h"
#include "lwip/netif.h"

/* Exported functions */
err_t ethernetif_init(struct netif *netif);
void  ethernetif_input(struct netif *netif);
void  ethernet_link_check_state(struct netif *netif);

#endif /* __ETHERNETIF_H__ */
