#ifndef SIXLOWAPP_H
#define SIXLOWAPP_H

#include "kernel.h"
#include "ipv6.h"

/**
 * @brief   The application version number
 */
#define APP_VERSION "1.3"

/**
 * @brief   Which interface should be used for 6LoWPAN
 */
#define IF_ID   (0)

/**
 * @brief   Define a IPC message type for ICMP echo reply handling
 */
#define ICMP_ECHO_REPLY_RCVD    (4444)

/**
 * @brief   PID for UDP server thread
 */
extern kernel_pid_t sixlowapp_udp_server_pid;

/**
 * @brief   UDP port number that netcat uses to listen at
 */
extern uint16_t sixlowapp_netcat_listen_port;

/**
 * @brief   Marker if we're waiting for an ICMP echo reply
 */
extern unsigned sixlowapp_waiting_for_pong;

/**
 * @brief   The PID of the thread waiting for the ICMP echo reply
 */
extern kernel_pid_t sixlowapp_waiter_pid;

/**
 * @brief   Helper variable for IP address printing
 */
extern char addr_str[IPV6_MAX_ADDR_STR_LEN];

/**
 * @brief            Shell command to send an ICMPv6 echo request
 *
 * @param[in] argc   Number of arguments supplied to the function invocation.
 * @param[in] argv   The supplied argument list.
 *
 */
int sixlowapp_send_ping(int argc, char **argv);

/**
 * @brief               Monitoring thread
 *
 * @param[in] unused    Obsolete
 */
void *sixlowapp_monitor(void *unused);

/**
 * @brief           UDP server thread
 *
 * @param[in] arg   Obsolete
 */
void *sixlowapp_udp_server_loop(void *arg);

/**
 * @brief               Waits for a certain message type for a given time span
 *
 * @param[out] m        Pointer to preallocated ``msg_t`` structure, must not
 *                      be NULL
 * @param[in] timeout   The maximum interval to wait
 * @param[in] mtype     The message type to wait for
 *
 * @return  0 if no message of type @p mtype was received
 * @return  The number of microseconds before the message was received
 */
uint64_t sixlowapp_wait_for_msg_type(msg_t *m, timex_t timeout, uint16_t mtype);

#endif /* SIXLOWAPP_H */
