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
 * @brief       IoT-Kit UDP example application
 *              (based on RIOT's POSIX sockets example).
 *
 * @author      Johann Fischer <j.fischer@phytec.de>
 * @author      Martine Lenders <mlenders@inf.fu-berlin.de>
 *
 * @}
 */


#include <stdio.h>

#include "shell.h"
#include "thread.h"
#include "xtimer.h"

#include "board.h"
#include "hdc1000.h"
#include "tmp006.h"
#include "mpl3115a2.h"
#include "mag3110.h"
#include "mma8652.h"
#include "tcs37727.h"
#include "periph_conf.h"
#include "periph/gpio.h"

extern int udp_init(char *port_str);

#define UDP_SERVER_PORT           "23025"

static uint8_t sensor_stat;

#define SENSOR_ENABLED_HDC1000    0x01
#define SENSOR_ENABLED_TMP006     0x02
#define SENSOR_ENABLED_MPL3115    0x04
#define SENSOR_ENABLED_MAG3110    0x08
#define SENSOR_ENABLED_MMA8652    0x10
#define SENSOR_ENABLED_TCS37727   0x20

static hdc1000_t hdc1000_dev;
static tmp006_t tmp006_dev;
static mpl3115a2_t mpl3115a2_dev;
static mag3110_t mag3110_dev;
static mma8652_t mma8652_dev;
static tcs37727_t tcs37727_dev;
static tcs37727_data_t tcs37727_data;

int temp, hum;
float tamb, tobj;
uint32_t pressure;
int16_t m_x, m_y, m_z;
int16_t a_x, a_y, a_z;
uint32_t ct, lux;

static char sensor_server_stack_buffer[512];

void sensors_init(void)
{
    sensor_stat = 0;

    if (hdc1000_init(&hdc1000_dev, I2C_0, 0x43) == 0) {
        sensor_stat |= SENSOR_ENABLED_HDC1000;
    }

    if (tmp006_init(&tmp006_dev, I2C_0, 0x41, TMP006_CONFIG_CR_DEF) == 0) {
        sensor_stat |= SENSOR_ENABLED_TMP006;
        if (tmp006_set_active(&tmp006_dev)) {
            sensor_stat &= ~SENSOR_ENABLED_TMP006;
        }
    }

    if (mpl3115a2_init(&mpl3115a2_dev, I2C_0, 0x60, MPL3115A2_OS_RATIO_DEFAULT) == 0) {
        sensor_stat |= SENSOR_ENABLED_MPL3115;
        if (mpl3115a2_set_active(&mpl3115a2_dev)) {
            sensor_stat &= ~SENSOR_ENABLED_MPL3115;
        }
    }

    if (mag3110_init(&mag3110_dev, I2C_0, 0x0e, MAG3110_DROS_DEFAULT) == 0) {
        sensor_stat |= SENSOR_ENABLED_MAG3110;
        if (mag3110_set_active(&mag3110_dev)) {
            sensor_stat &= ~SENSOR_ENABLED_MAG3110;
        }
    }

    if (mma8652_init(&mma8652_dev, I2C_0, 0x1d, MMA8652_DATARATE_DEFAULT,
                     MMA8652_FS_RANGE_DEFAULT) == 0) {
        sensor_stat |= SENSOR_ENABLED_MMA8652;
        if (mma8652_set_active(&mma8652_dev)) {
            sensor_stat &= ~SENSOR_ENABLED_MMA8652;
        }
    }

    if (tcs37727_init(&tcs37727_dev, I2C_0, 0x29,
                      TCS37727_ATIME_DEFAULT) == 0) {
        sensor_stat |= SENSOR_ENABLED_TCS37727;
        if (tcs37727_set_rgbc_active(&tcs37727_dev)) {
            sensor_stat &= ~SENSOR_ENABLED_TCS37727;
        }
    }

    printf("Sensor status: 0x%x\n", sensor_stat);
}

static void *sensor_server(void *arg)
{
    (void) arg;
    uint8_t status;

    sensors_init();

    LED_G_ON;
    LED_B_ON;
    LED_R_ON;
    xtimer_usleep(100000);
    LED_G_OFF;
    LED_B_OFF;
    LED_R_OFF;

    while (1) {
        if (sensor_stat & SENSOR_ENABLED_HDC1000) {
            uint16_t rawtemp, rawhum;

            if (hdc1000_startmeasure(&hdc1000_dev)) {
                sensor_stat &= ~SENSOR_ENABLED_HDC1000;
            }
            xtimer_usleep(HDC1000_CONVERSION_TIME);

            hdc1000_read(&hdc1000_dev, &rawtemp, &rawhum);
            hdc1000_convert(rawtemp, rawhum,  &temp, &hum);
        }

        if (sensor_stat & SENSOR_ENABLED_TMP006) {
            int16_t rawtemp, rawvolt;
            uint8_t drdy;

            tmp006_read(&tmp006_dev, &rawvolt, &rawtemp, &drdy);
            if (drdy) {
                tmp006_convert(rawvolt, rawtemp,  &tamb, &tobj);
            }
        }

        if (sensor_stat & SENSOR_ENABLED_MPL3115) {
            mpl3115a2_read_pressure(&mpl3115a2_dev, &pressure, &status);
        }

        if (sensor_stat & SENSOR_ENABLED_MAG3110) {
            mag3110_read(&mag3110_dev, &m_x, &m_y, &m_z, &status);
        }

        if (sensor_stat & SENSOR_ENABLED_MMA8652) {
            mma8652_read(&mma8652_dev, &a_x, &a_y, &a_z, &status);
        }

        if (sensor_stat & SENSOR_ENABLED_TCS37727) {
            tcs37727_read(&tcs37727_dev, &tcs37727_data);
            ct = tcs37727_data.ct;
            lux = tcs37727_data.lux;
        }
        xtimer_usleep(1*1000000);
    }

    return NULL;
}

int main(void)
{
    puts("phyNODE udp example application");

    thread_create(sensor_server_stack_buffer,
                  sizeof(sensor_server_stack_buffer),
                  THREAD_PRIORITY_MAIN - 1,
                  CREATE_STACKTEST,
                  sensor_server,
                  NULL,
                  "sensor server");

    udp_init(UDP_SERVER_PORT);

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}
