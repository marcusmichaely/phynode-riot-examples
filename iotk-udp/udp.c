/*
 * Copyright (C) 2015 PHYTEC Messtechnik GmbH
 * Copyright (C) 2015 Martine Lenders <mlenders@inf.fu-berlin.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Demonstrating the sending and receiving of sensor
 *              data over UDP and POSIX sockets.
 *
 * @author      Johann Fischer <j.fischer@phytec.de>
 * @author      Martine Lenders <mlenders@inf.fu-berlin.de>
 *
 * @}
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "thread.h"
#include "board.h"

#define SERVER_MSG_QUEUE_SIZE   (8)
#define UDP_BUF_SIZE            (64)

static int server_socket = -1;
static char udp_buf[UDP_BUF_SIZE];
static char server_stack[THREAD_STACKSIZE_DEFAULT];
static msg_t server_msg_queue[SERVER_MSG_QUEUE_SIZE];

extern int temp, hum;
extern float tamb, tobj;
extern uint32_t pressure;
extern int16_t m_x, m_y, m_z;
extern int16_t a_x, a_y, a_z;
extern uint32_t ct, lux;

static void *_server_thread(void *args)
{
    struct sockaddr_in6 server_addr;
    struct sockaddr_in6 dst_addr;
    int dst_socket;
    uint16_t port;

    msg_init_queue(server_msg_queue, SERVER_MSG_QUEUE_SIZE);

    server_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    dst_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    /* parse port */
    port = (uint16_t)atoi((char *)args);
    if (port == 0) {
        puts("Error: invalid port specified");
        return NULL;
    }
    server_addr.sin6_family = AF_INET6;
    dst_addr.sin6_family = AF_INET6;

    memset(&server_addr.sin6_addr, 0, sizeof(server_addr.sin6_addr));
    memset(&dst_addr.sin6_addr, 0, sizeof(server_addr.sin6_addr));

    server_addr.sin6_port = htons(port);
    dst_addr.sin6_port = htons(port);

    if (server_socket < 0) {
        puts("error initializing socket");
        server_socket = 0;
        return NULL;
    }
    if (dst_socket < 0) {
        puts("error initializing client socket");
        dst_socket = 0;
        return NULL;
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        server_socket = -1;
        puts("error binding socket");
        return NULL;
    }
    printf("Success: started UDP server on port %" PRIu16 "\n", port);

    while (1) {
        int res;
        struct sockaddr_in6 src;
        socklen_t src_len = sizeof(struct sockaddr_in6);
        if ((res = recvfrom(server_socket, udp_buf, sizeof(udp_buf), 0,
                            (struct sockaddr *)&src, &src_len)) < 0) {
            puts("Error on receive");
        }
        else if (res == 0) {
            puts("Peer did shut down");
        }
        else {
            /*
            printf("Received data: ");
            puts(udp_buf);
            */
	    if (strstr(udp_buf, "get:hdc1000") != NULL) {
                snprintf(udp_buf, UDP_BUF_SIZE, "temp:%d,rh:%d\n", temp, hum);
            } else if (strstr(udp_buf, "get:mpl3115a2") != NULL) {
                snprintf(udp_buf, UDP_BUF_SIZE, "pressure:%d\n", (unsigned int)pressure);
            } else if (strstr(udp_buf, "get:mma8652") != NULL) {
                snprintf(udp_buf, UDP_BUF_SIZE, "x:%d,y:%d,z:%d\n", a_x, a_y, a_z);
            } else if (strstr(udp_buf, "get:mag3110") != NULL) {
                snprintf(udp_buf, UDP_BUF_SIZE, "x:%d,y:%d,z:%d\n", m_x, m_y, m_z);
            } else if (strstr(udp_buf, "get:tmp006") != NULL) {
                snprintf(udp_buf, UDP_BUF_SIZE, "tamb:%d,tobj:%d\n",
                         (int)(tamb*100), (int)(tobj*100));
            } else if (strstr(udp_buf, "get:tcs37727") != NULL) {
                snprintf(udp_buf, UDP_BUF_SIZE, "ct:%d,lux:%d\n",
                         (int)ct, (int)lux);
            } else if (strstr(udp_buf, "set:rled,val:0") != NULL ||
	    		(strstr(udp_buf, "set:rled,val:00") != NULL)) {
	    	LED_R_OFF;
	    	snprintf(udp_buf, UDP_BUF_SIZE, "LED_R_OFF\n");
            } else if (strstr(udp_buf, "set:rled,val:") != NULL) {
	    	LED_R_ON;
	    	snprintf(udp_buf, UDP_BUF_SIZE, "LED_R_ON\n");
            } else if (strstr(udp_buf, "set:gled,val:0") != NULL ||
	    		(strstr(udp_buf, "set:gled,val:00") != NULL)) {
	    	LED_G_OFF;
	    	snprintf(udp_buf, UDP_BUF_SIZE, "LED_G_OFF\n");
            } else if (strstr(udp_buf, "set:gled,val:") != NULL) {
	    	LED_G_ON;
	    	snprintf(udp_buf, UDP_BUF_SIZE, "LED_G_ON\n");
            } else if (strstr(udp_buf, "set:bled,val:0") != NULL ||
	    		(strstr(udp_buf, "set:bled,val:00") != NULL)) {
	    	LED_B_OFF;
	    	snprintf(udp_buf, UDP_BUF_SIZE, "LED_B_OFF\n");
            } else if (strstr(udp_buf, "set:bled,val:") != NULL) {
	    	LED_B_ON;
	    	snprintf(udp_buf, UDP_BUF_SIZE, "LED_B_ON\n");
            } else {
                continue;
            }

            memcpy(&dst_addr.sin6_addr, &src.sin6_addr, 16);

            sendto(dst_socket, udp_buf, strlen(udp_buf), 0,
                   (struct sockaddr *)&dst_addr, sizeof(dst_addr));
        }
    }
    return NULL;
}

static int udp_start_server(char *port_str)
{
    /* check if server is already running */
    if (server_socket >= 0) {
        puts("Error: server already running");
        return 1;
    }
    /* start server (which means registering pktdump for the chosen port) */
    if (thread_create(server_stack, sizeof(server_stack), THREAD_PRIORITY_MAIN - 1,
                      THREAD_CREATE_STACKTEST,
                      _server_thread, port_str, "UDP server") <= KERNEL_PID_UNDEF) {
        server_socket = -1;
        puts("error initializing thread");
        return 1;
    }
    return 0;
}

int udp_init(char *port_str)
{
    return udp_start_server(port_str);
}

/** @} */
