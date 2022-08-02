/*
 * Copyright (c) 2022 Hunan OpenValley Digital Industry Development Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include "sdkconfig.h"

#define SYS_LIGHTWEIGHT_PROT            (1)
#define MEMCPY                          memcpy
#define SMEMCPY                         memcpy

#define LWIP_RAND                       esp_random
#define MEM_LIBC_MALLOC                 (1)
#define MEMP_MEM_MALLOC                 (1)
#define MEM_ALIGNMENT                   (4)
#define MEMP_NUM_NETCONN                CONFIG_LWIP_MAX_SOCKETS
#define MEMP_NUM_RAW_PCB                CONFIG_LWIP_MAX_RAW_PCBS
#define MEMP_NUM_TCP_PCB                CONFIG_LWIP_MAX_ACTIVE_TCP
#define MEMP_NUM_TCP_PCB_LISTEN         CONFIG_LWIP_MAX_LISTENING_TCP
#define MEMP_NUM_UDP_PCB                CONFIG_LWIP_MAX_UDP_PCBS
#define ARP_QUEUEING                    (1)
#define IP_REASSEMBLY                   CONFIG_LWIP_IP4_REASSEMBLY
#define LWIP_IPV6_REASS                 CONFIG_LWIP_IP6_REASSEMBLY
#define IP_FRAG                         CONFIG_LWIP_IP4_FRAG
#define LWIP_IPV6_FRAG                  CONFIG_LWIP_IP6_FRAG
#define IP_REASS_MAXAGE                 (3)
#define IP_REASS_MAX_PBUFS              (10)
#define IP_FORWARD                      CONFIG_LWIP_IP_FORWARD
#define IP_NAPT                         CONFIG_LWIP_IPV4_NAPT
#define LWIP_ICMP  CONFIG_LWIP_ICMP
#define LWIP_BROADCAST_PING CONFIG_LWIP_BROADCAST_PING
#define LWIP_MULTICAST_PING CONFIG_LWIP_MULTICAST_PING
#define LWIP_RAW                        (1)
#define LWIP_DHCP                       (1)
#define DHCP_MAXRTX                     (0)
#define DHCP_DOES_ARP_CHECK             CONFIG_LWIP_DHCP_DOES_ARP_CHECK
#define ESP_DHCP_DISABLE_CLIENT_ID      CONFIG_LWIP_DHCP_DISABLE_CLIENT_ID
#if CONFIG_LWIP_DHCP_RESTORE_LAST_IP
#define LWIP_DHCP_IP_ADDR_RESTORE       dhcp_ip_addr_restore
#define LWIP_DHCP_IP_ADDR_STORE         dhcp_ip_addr_store
#define LWIP_DHCP_IP_ADDR_ERASE         dhcp_ip_addr_erase
#endif

#ifdef CONFIG_LWIP_AUTOIP
#define LWIP_AUTOIP                     (1)
#define LWIP_DHCP_AUTOIP_COOP           (1)
#define LWIP_DHCP_AUTOIP_COOP_TRIES     CONFIG_LWIP_AUTOIP_TRIES
#define LWIP_AUTOIP_MAX_CONFLICTS CONFIG_LWIP_AUTOIP_MAX_CONFLICTS
#define LWIP_AUTOIP_RATE_LIMIT_INTERVAL CONFIG_LWIP_AUTOIP_RATE_LIMIT_INTERVAL
#endif

#define LWIP_IGMP                       (1)
#define LWIP_DNS                        (1)
#define DNS_MAX_SERVERS                 (3)
#define DNS_FALLBACK_SERVER_INDEX       (DNS_MAX_SERVERS - 1)
#define TCP_QUEUE_OOSEQ                 CONFIG_LWIP_TCP_QUEUE_OOSEQ
#define LWIP_TCP_SACK_OUT               CONFIG_LWIP_TCP_SACK_OUT
#define ESP_TCP_KEEP_CONNECTION_WHEN_IP_CHANGES  CONFIG_LWIP_TCP_KEEP_CONNECTION_WHEN_IP_CHANGES
#define TCP_MSS                         CONFIG_LWIP_TCP_MSS
#define TCP_TMR_INTERVAL                CONFIG_LWIP_TCP_TMR_INTERVAL
#define TCP_MSL                         CONFIG_LWIP_TCP_MSL
#define TCP_MAXRTX                      CONFIG_LWIP_TCP_MAXRTX
#define TCP_SYNMAXRTX                   CONFIG_LWIP_TCP_SYNMAXRTX
#define TCP_LISTEN_BACKLOG              (1)
#ifdef CONFIG_LWIP_TCP_OVERSIZE_MSS
#define TCP_OVERSIZE                    TCP_MSS
#endif
#ifdef CONFIG_LWIP_TCP_OVERSIZE_QUARTER_MSS
#define TCP_OVERSIZE                    (TCP_MSS/4)
#endif
#ifdef CONFIG_LWIP_TCP_OVERSIZE_DISABLE
#define TCP_OVERSIZE                    (0)
#endif
#ifndef TCP_OVERSIZE
#error "One of CONFIG_TCP_OVERSIZE_xxx options should be set by sdkconfig"
#endif
#ifdef CONFIG_LWIP_WND_SCALE
#define LWIP_WND_SCALE                  (1)
#define TCP_RCV_SCALE                   CONFIG_LWIP_TCP_RCV_SCALE
#endif
#define LWIP_TCP_RTO_TIME               CONFIG_LWIP_TCP_RTO_TIME
#define LWIP_NETIF_HOSTNAME             (1)
#define LWIP_NETIF_TX_SINGLE_PBUF       (1)
#ifdef CONFIG_LWIP_NETIF_LOOPBACK
#define LWIP_NETIF_LOOPBACK             (1)
#define LWIP_LOOPBACK_MAX_PBUFS         CONFIG_LWIP_LOOPBACK_MAX_PBUFS
#endif
#ifdef CONFIG_LWIP_SLIP_SUPPORT
#define SLIP_RX_FROM_ISR                 (1)
#define SLIP_USE_RX_THREAD               (0)
#define SLIP_DEBUG_ON                    CONFIG_LWIP_SLIP_DEBUG_ON
#if SLIP_DEBUG_ON
#define SLIP_DEBUG                       LWIP_DBG_ON
#else
#define SLIP_DEBUG                       LWIP_DBG_OFF
#endif
#endif
#define TCPIP_THREAD_NAME              "tiT"
#define TCPIP_THREAD_STACKSIZE          (4096)
#define TCPIP_THREAD_PRIO               (25-7)
#define TCPIP_MBOX_SIZE                 CONFIG_LWIP_TCPIP_RECVMBOX_SIZE
#define DEFAULT_UDP_RECVMBOX_SIZE       CONFIG_LWIP_UDP_RECVMBOX_SIZE
#define DEFAULT_TCP_RECVMBOX_SIZE       CONFIG_LWIP_TCP_RECVMBOX_SIZE
#define DEFAULT_ACCEPTMBOX_SIZE         (6)
#define DEFAULT_THREAD_STACKSIZE        TCPIP_THREAD_STACKSIZE
#define DEFAULT_THREAD_PRIO             TCPIP_THREAD_PRIO
#define DEFAULT_RAW_RECVMBOX_SIZE       (6)
#define LWIP_TCPIP_CORE_LOCKING         (0)
#define LWIP_SO_SNDTIMEO                (1)
#define LWIP_SO_RCVTIMEO                (1)
#define LWIP_TCP_KEEPALIVE              (1)
#define LWIP_SO_LINGER                  CONFIG_LWIP_SO_LINGER
#define LWIP_SO_RCVBUF                  CONFIG_LWIP_SO_RCVBUF
#define SO_REUSE                        CONFIG_LWIP_SO_REUSE
#define LWIP_DNS_SUPPORT_MDNS_QUERIES   CONFIG_LWIP_DNS_SUPPORT_MDNS_QUERIES
#define SO_REUSE_RXTOALL                CONFIG_LWIP_SO_REUSE_RXTOALL
#define LWIP_NETBUF_RECVINFO            CONFIG_LWIP_NETBUF_RECVINFO
#define LWIP_STATS                      CONFIG_LWIP_STATS
#if LWIP_STATS
#define LWIP_STATS_DISPLAY              CONFIG_LWIP_STATS
#endif
#define PPP_SUPPORT                     CONFIG_LWIP_PPP_SUPPORT
#if PPP_SUPPORT
#define PPP_IPV6_SUPPORT				CONFIG_LWIP_PPP_ENABLE_IPV6
#define PPP_NOTIFY_PHASE                CONFIG_LWIP_PPP_NOTIFY_PHASE_SUPPORT
#define PAP_SUPPORT                     CONFIG_LWIP_PPP_PAP_SUPPORT
#define CHAP_SUPPORT                    CONFIG_LWIP_PPP_CHAP_SUPPORT
#define MSCHAP_SUPPORT                  CONFIG_LWIP_PPP_MSCHAP_SUPPORT
#define MPPE_SUPPORT                    CONFIG_LWIP_PPP_MPPE_SUPPORT
#define PPP_MAXIDLEFLAG                 (0)
#define PPP_DEBUG_ON                    CONFIG_LWIP_PPP_DEBUG_ON
#if PPP_DEBUG_ON
#define PPP_DEBUG                       LWIP_DBG_ON
#define PRINTPKT_SUPPORT                (1)
#define PPP_PROTOCOLNAME                (1)
#else
#define PPP_DEBUG                       LWIP_DBG_OFF
#endif
#endif
#define LWIP_IPV6                       (0)
#define MEMP_NUM_ND6_QUEUE              (0)
#define LWIP_ND6_NUM_NEIGHBORS          (0)
#define LWIP_HOOK_FILENAME              "lwip_default_hooks.h"
#ifdef CONFIG_LWIP_ETHARP_DEBUG
#define ETHARP_DEBUG                     LWIP_DBG_ON
#else
#define ETHARP_DEBUG                     LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_NETIF_DEBUG
#define NETIF_DEBUG                     LWIP_DBG_ON
#else
#define NETIF_DEBUG                     LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_PBUF_DEBUG
#define PBUF_DEBUG                     LWIP_DBG_ON
#else
#define PBUF_DEBUG                     LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_API_LIB_DEBUG
#define API_LIB_DEBUG                     LWIP_DBG_ON
#else
#define API_LIB_DEBUG                     LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_SOCKETS_DEBUG
#define SOCKETS_DEBUG                   LWIP_DBG_ON
#else
#define SOCKETS_DEBUG                   LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_ICMP_DEBUG
#define ICMP_DEBUG                      LWIP_DBG_ON
#else
#define ICMP_DEBUG                      LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_ICMP6_DEBUG
#define ICMP6_DEBUG                      LWIP_DBG_ON
#else
#define ICMP6_DEBUG                      LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_DHCP_DEBUG
#define DHCP_DEBUG                      LWIP_DBG_ON
#else
#define DHCP_DEBUG                      LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_DHCP_STATE_DEBUG
#define ESP_DHCP_DEBUG                  LWIP_DBG_ON
#else
#define ESP_DHCP_DEBUG                  LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_IP_DEBUG
#define IP_DEBUG                        LWIP_DBG_ON
#else
#define IP_DEBUG                        LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_IP6_DEBUG
#define IP6_DEBUG                        LWIP_DBG_ON
#else
#define IP6_DEBUG                        LWIP_DBG_OFF
#endif
#ifdef CONFIG_LWIP_TCP_DEBUG
#define TCP_DEBUG                        LWIP_DBG_ON
#else
#define TCP_DEBUG                        LWIP_DBG_OFF
#endif
#define MEMP_DEBUG                      LWIP_DBG_OFF
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF
#define TCPIP_DEBUG                     LWIP_DBG_OFF
#define TCP_OOSEQ_DEBUG                 LWIP_DBG_OFF
#define ETHARP_TRUST_IP_MAC             CONFIG_LWIP_ETHARP_TRUST_IP_MAC
#define LWIP_POSIX_SOCKETS_IO_NAMES     (0)
#define LWIP_SOCKET_OFFSET              (FD_SETSIZE - CONFIG_LWIP_MAX_SOCKETS)
#define ESP_LWIP                        (1)
#define ESP_LWIP_ARP                    (0)
#define ESP_PER_SOC_TCP_WND             (0)
#define ESP_THREAD_SAFE                 (0)
#define ESP_THREAD_SAFE_DEBUG           LWIP_DBG_OFF
#define ESP_DHCP                        (0)
#define ESP_DNS                         (0)
#define ESP_PERF                        (0)
#define ESP_RANDOM_TCP_PORT             (1)
#define ESP_IP4_ATON                    (1)
#define ESP_LIGHT_SLEEP                 (1)
#define ESP_L2_TO_L3_COPY               CONFIG_LWIP_L2_TO_L3_COPY
#define ESP_STATS_MEM                   CONFIG_LWIP_STATS
#define ESP_STATS_DROP                  CONFIG_LWIP_STATS
#define ESP_STATS_TCP                   (0)
#define ESP_LWIP_LOGI(...)
#define ESP_PING                        (1)
#define ESP_HAS_SELECT                  (1)
#define ESP_AUTO_RECV                   (1)
#define ESP_PBUF                        (1)
#define ESP_IPV6                        (0)
#define ESP_SOCKET                      (1)
#define ESP_LWIP_SELECT                 (1)
#define ESP_THREAD_PROTECTION           (0)
#define ESP_IRAM_ATTR
#define ESP_LWIP_IGMP_TIMERS_ONDEMAND   (1)
#define ESP_LWIP_MLD6_TIMERS_ONDEMAND   (1)
#define TCP_SND_BUF                     CONFIG_LWIP_TCP_SND_BUF_DEFAULT
#define TCP_WND                         CONFIG_LWIP_TCP_WND_DEFAULT
#ifdef CONFIG_LWIP_DEBUG
#define LWIP_DEBUG                      LWIP_DBG_ON
#else
#undef LWIP_DEBUG
#endif
#define CHECKSUM_CHECK_UDP              CONFIG_LWIP_CHECKSUM_CHECK_UDP
#define CHECKSUM_CHECK_IP               CONFIG_LWIP_CHECKSUM_CHECK_IP
#define CHECKSUM_CHECK_ICMP             CONFIG_LWIP_CHECKSUM_CHECK_ICMP
#define LWIP_NETCONN_FULLDUPLEX         (1)
#define LWIP_NETCONN_SEM_PER_THREAD     (1)
#define LWIP_DHCP_MAX_NTP_SERVERS       CONFIG_LWIP_DHCP_MAX_NTP_SERVERS
#define LWIP_TIMEVAL_PRIVATE            (0)
#ifdef __cplusplus
#define LWIP_FORWARD_DECLARE_C_CXX extern "C"
#else
#define LWIP_FORWARD_DECLARE_C_CXX
#endif

#define SNTP_SERVER_DNS                (1)
#define SNTP_SUPPRESS_DELAY_CHECK

#define SOC_SEND_LOG
#undef MQTT_OUTPUT_RINGBUF_SIZE
#define MQTT_OUTPUT_RINGBUF_SIZE       (1024)
#undef MQTT_VAR_HEADER_BUFFER_LEN
#define MQTT_VAR_HEADER_BUFFER_LEN     (1024)
#undef MQTT_REQ_MAX_IN_FLIGHT
#define MQTT_REQ_MAX_IN_FLIGHT         (15)
#undef MQTT_REQ_TIMEOUT
#define MQTT_REQ_TIMEOUT               (30)
#undef MQTT_CONNECT_TIMOUT
#define MQTT_CONNECT_TIMOUT            (100)

#if LWIP_NETCONN_SEM_PER_THREAD
#define LWIP_NETCONN_THREAD_SEM_GET    sys_thread_sem_get
#define LWIP_NETCONN_THREAD_SEM_ALLOC  sys_thread_sem_init
#define LWIP_NETCONN_THREAD_SEM_FREE   sys_thread_sem_deinit
#endif

#endif
