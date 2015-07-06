/*
 * Copyright (C) 2013, 2014 INRIA
 * Copyright (C) 2015 PHYTEC Messtechnik GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @file
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 * @author      Johann Fischer <j.fischer@phytec.de>
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "thread.h"
#include "socket_base/socket.h"
#include "net_help.h"
#include "sixlowapp.h"

extern int temp, hum;
extern float tamb, tobj;
extern uint32_t pressure;
extern int16_t m_x, m_y, m_z;
extern int16_t a_x, a_y, a_z;

#define UDP_BUF_SIZE     (128)
#define SERVER_PORT     (0x59F1)

/* UDP server thread */
void *sixlowapp_udp_server_loop(void *arg)
{
    (void) arg;

    sockaddr6_t sa_in;
    sockaddr6_t sa_out;
    char udp_buf[UDP_BUF_SIZE];
    uint32_t fromlen;
    int sock_in = socket_base_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    int sock_out = socket_base_socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    int bytes_sent;

    memset(&sa_in, 0, sizeof(sa_in));
    memset(&sa_out, 0, sizeof(sa_out));

    sa_in.sin6_family = AF_INET;
    sa_in.sin6_port = HTONS(SERVER_PORT);
    sa_out.sin6_family = AF_INET;
    sa_out.sin6_port = HTONS(SERVER_PORT);

    fromlen = sizeof(sa_in);

    if (-1 == socket_base_bind(sock_in, &sa_in, sizeof(sa_in))) {
        printf("Error bind failed!\n");
        socket_base_close(sock_in);
        return NULL;
    }

    if (-1 == sock_out) {
        printf("Error Creating Socket failed!\n");
        return NULL;
    }

    while (1) {
        int32_t recsize = socket_base_recvfrom(sock_in,
                                               (void *)udp_buf,
                                               UDP_BUF_SIZE, 0,
                                               &sa_in, &fromlen);

        if (recsize < 0) {
            printf("ERROR: recsize < 0!\n");
        }

        /* printf("UDP packet received, payload: %s\n", udp_buf); */

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

        memcpy(&sa_out.sin6_addr, &sa_in.sin6_addr, 16);

        bytes_sent = socket_base_sendto(sock_out, (char *)udp_buf,
                                           strlen(udp_buf) + 1, 0, &sa_out,
                                           sizeof(sa_out));

        if (bytes_sent < 0) {
            printf("Error sending packet!\n");
        }
    }

    socket_base_close(sock_in);
    socket_base_close(sock_out);

    return NULL;
}
