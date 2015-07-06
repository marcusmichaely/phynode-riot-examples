/*
 * Copyright (C) 2014 INRIA
 * Copyright (C) 2015 PHYTEC Messtechnik GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @file
 * @brief       IoT-Kit UDP example application - main function
 *
 * @author      Oliver Hahm <oliver.hahm@inria.fr>
 * @author      Johann Fischer <j.fischer@phytec.de>
 */

#include <stdio.h>

#include "kernel.h"
#include "thread.h"
#include "net_if.h"
#include "posix_io.h"
#include "shell.h"
#include "shell_commands.h"
#include "board_uart0.h"
#include "transceiver.h"

#include "hdc1000.h"
#include "tmp006.h"
#include "mpl3115a2.h"
#include "mag3110.h"
#include "mma8652.h"
#include "periph_conf.h"
#include "periph/gpio.h"

#include "sixlowapp.h"

#define RCV_BUFFER_SIZE     (32)

#define UNASSIGNED_CHANNEL INT_MIN

kernel_pid_t hack_msg_pid;

static uint8_t sensor_stat;
static int gpiottest_initialized;

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

int temp, hum;
float tamb, tobj;
uint32_t pressure;
int16_t m_x, m_y, m_z;
int16_t a_x, a_y, a_z;

kernel_pid_t sixlowapp_udp_server_pid = KERNEL_PID_UNDEF;

char addr_str[IPV6_MAX_ADDR_STR_LEN];
char monitor_stack_buffer[KERNEL_CONF_STACKSIZE_MAIN];
char udp_server_stack_buffer[KERNEL_CONF_STACKSIZE_MAIN];

int cmd_gpio_test (int argc, char **argv);

static const shell_command_t shell_commands[] = {
    {"ping", "Send an ICMPv6 echo request to another node", sixlowapp_send_ping},
    {"gpiotest", "Run GPIO test", cmd_gpio_test},
    {NULL, NULL, NULL}
};

static char sensor_server_stack_buffer[KERNEL_CONF_STACKSIZE_MAIN];

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
    printf("sensor_stat: 0x%x\n", sensor_stat);
}

static void *sensor_server(void *arg)
{
    (void) arg;
    uint8_t status;

    sensors_init();

    timex_t sleep = timex_set(1, 0);

    LED_G_ON;
    LED_B_ON;
    LED_R_ON;
    vtimer_usleep(100000);
    LED_G_OFF;
    LED_B_OFF;
    LED_R_OFF;

    while (1) {
        if (sensor_stat & SENSOR_ENABLED_HDC1000) {
            uint16_t rawtemp, rawhum;

            if (hdc1000_startmeasure(&hdc1000_dev)) {
                sensor_stat &= ~SENSOR_ENABLED_HDC1000;
            }
            vtimer_usleep(HDC1000_CONVERSION_TIME);

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

        vtimer_sleep(sleep);
    }

    return NULL;
}

int cmd_gpiotest_init(int argc, char **argv)
{
    for (int i = 0; i < GPIO_NUMOF-2; i++) {
        gpio_init_out(i, GPIO_NOPULL);
        gpio_clear(i);
    }
    gpiottest_initialized = 1;
    return 0;
}

int cmd_gpio_test (int argc, char **argv)
{
    int gpio = -1;
    int state = -1;

    if (argc != 3) {
        puts("");
        puts("usage: gpiotest GPIO 1|0");
        return -1;
    }

    gpio = atoi(argv[1]);
    if (gpio < 0 || gpio >= (GPIO_NUMOF-2)) {
        puts("error: invalid GPIO value given");
        return -1;
    }

    state = atoi(argv[2]);
    if ((state < 0) || (state > 1)) {
        puts("error: invalid state value given");
        return -1;
    }

    if (!gpiottest_initialized) {
        cmd_gpiotest_init(1, NULL);
    }

    if (state) {
        puts("turning a pin on");
        gpio_set(gpio);
    }
    else {
        puts("turning a pin off");
        gpio_clear(gpio);
    }

    /*
    for (int i = 0; i < GPIO_NUMOF-2; i++) {
        gpio_set(i);
        vtimer_usleep(400000);
        gpio_clear(i);
    }
    vtimer_usleep(200000);
    reboot(RB_AUTOBOOT);
    */
    return 0;
}

int main(void)
{
    int32_t chan = 13;
    msg_t m;
    transceiver_command_t tcmd;

    puts("RIOT 6LoWPAN example v"APP_VERSION);

    tcmd.transceivers = TRANSCEIVER_DEFAULT;
    tcmd.data = &chan;
    m.type = SET_CHANNEL;
    m.content.ptr = (void *) &tcmd;
    msg_send_receive(&m, &m, transceiver_pid);
    if( chan == UNASSIGNED_CHANNEL ) {
        puts("ERROR: channel NOT set!");
    }

    net_if_set_src_address_mode(0, NET_IF_TRANS_ADDR_M_LONG);
    net_if_set_hardware_address(0, net_if_get_hardware_address(0));

    sixlowpan_lowpan_init_interface(IF_ID);

    /* add global address */
    ipv6_addr_t tmp;
    /* initialize prefix */
    ipv6_addr_init(&tmp, 0xfe80, 0x0, 0x0, 0x0, 0xfdfe, 0x0, 0x0, 0x9f);
    /* set host suffix */
    /*ipv6_addr_set_by_eui64(&tmp, 0, &tmp);*/
    ipv6_net_if_add_addr(0, &tmp, NDP_ADDR_STATE_PREFERRED, 0, 0, 0);

    /* start thread for monitor mode */
    kernel_pid_t monitor_pid = thread_create(monitor_stack_buffer,
                                             sizeof(monitor_stack_buffer),
                                             PRIORITY_MAIN - 2,
                                             CREATE_STACKTEST,
                                             sixlowapp_monitor, NULL,
                                             "monitor");

    ipv6_register_packet_handler(monitor_pid);

    /* Start the UDP server thread */
    sixlowapp_udp_server_pid = thread_create(udp_server_stack_buffer,
                                             sizeof(udp_server_stack_buffer),
                                             PRIORITY_MAIN, CREATE_STACKTEST,
                                             sixlowapp_udp_server_loop, NULL,
                                             "UDP receiver");

    thread_create(
                  sensor_server_stack_buffer,
                  sizeof(sensor_server_stack_buffer),
                  PRIORITY_MAIN - 1,
                  CREATE_STACKTEST,
                  sensor_server,
                  NULL,
                  "sensor server");

    /* Open the UART0 for the shell */
    posix_open(uart0_handler_pid, 0);
    /* initialize the shell */
    shell_t shell;
    shell_init(&shell, shell_commands, UART0_BUFSIZE, uart0_readc, uart0_putc);
    /* start the shell loop */
    shell_run(&shell);

    return 0;
}
