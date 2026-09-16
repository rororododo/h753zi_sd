/**
  ******************************************************************************
  * @file    lwipopts.h
  * @brief   LwIP configuration options for STM32H753ZI (No-OS, Static Allocation)
  ******************************************************************************
  */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

/**
  * NO_SYS == 1: Pure Bare-Metal (No OS, Superloop)
  */
#define NO_SYS                          1
#define SYS_LIGHTWEIGHT_PROT            0

/* ---------- Memory options ---------- */
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)

/* STRICT STATIC ALLOCATION: No dynamic malloc / libc malloc */
#define MEM_LIBC_MALLOC                 0
#define MEMP_MEM_MALLOC                 0
#define MEM_USE_POOLS                   0

/* ---------- Internal Memory Pool Sizes ---------- */
#define MEMP_NUM_PBUF                   16
#define MEMP_NUM_RAW_PCB                4
#define MEMP_NUM_UDP_PCB                4
#define MEMP_NUM_TCP_PCB                4
#define MEMP_NUM_TCP_PCB_LISTEN         2
#define MEMP_NUM_TCP_SEG                16
#define MEMP_NUM_REASSDATA              4
#define MEMP_NUM_FRAG_PBUF              8
#define MEMP_NUM_ARP_QUEUE              8
#define MEMP_NUM_SYS_TIMEOUT            8

/* ---------- Pbuf options ---------- */
#define PBUF_POOL_SIZE                  16
#define PBUF_POOL_BUFSIZE               1536
#define PBUF_LINK_ENCAPSULATION_HLEN    0
#define LWIP_SUPPORT_CUSTOM_PBUF        1

/* ---------- ARP options ---------- */
#define LWIP_ARP                        1
#define ARP_TABLE_SIZE                  10
#define ARP_QUEUEING                    1

/* ---------- IP options ---------- */
#define LWIP_IPV4                       1
#define IP_FORWARD                      0
#define IP_OPTIONS_ALLOWED              1
#define IP_REASSEMBLY                   1
#define IP_FRAG                         1
#define IP_REASS_MAXAGE                 3
#define IP_REASS_MAX_PBUFS              4

/* ---------- ICMP options ---------- */
#define LWIP_ICMP                       1
#define ICMP_TTL                        255

/* ---------- DHCP options ---------- */
#define LWIP_DHCP                       0

/* ---------- UDP options ---------- */
#define LWIP_UDP                        1
#define UDP_TTL                         255

/* ---------- TCP options ---------- */
#define LWIP_TCP                        1
#define TCP_TTL                         255
#define TCP_WND                         (4 * TCP_MSS)
#define TCP_MAXRTX                      12
#define TCP_SYNMAXRTX                   4
#define TCP_QUEUE_OOSEQ                 0
#define TCP_MSS                         1460
#define TCP_SND_BUF                     (4 * TCP_MSS)
#define TCP_SND_QUEUELEN                (2 * TCP_SND_BUF/TCP_MSS)

/* ---------- Link / Status Callbacks ---------- */
#define LWIP_NETIF_STATUS_CALLBACK      1
#define LWIP_NETIF_LINK_CALLBACK        1

/* ---------- Checksum options (Software calculated for full safety) ---------- */
#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1

/* ---------- Sequential / Socket API (Disabled in NO_SYS) ---------- */
#define LWIP_NETCONN                    0
#define LWIP_SOCKET                     0

/* ---------- Statistics ---------- */
#define LWIP_STATS                      0

#endif /* __LWIPOPTS_H__ */
