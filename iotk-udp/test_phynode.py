#!/usr/bin/python
# Copyright (C) 2015 PHYTEC Messtechnik GmbH
#
# This file is subject to the terms and conditions of the GNU Lesser
# General Public License v2.1. See the file LICENSE in the top level
# directory for more details.

import socket
import time
import argparse

UDP_PORT = 23025

parser = argparse.ArgumentParser(description='Test the udp communication with the phyNODE')
parser.add_argument('addr', nargs=2, help='a IPv6 address of the phyNODE')
args = parser.parse_args()
print (args.addr)

sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
sock.bind(('', UDP_PORT))

def tx_expect( send_str, expect_str ):
        k=0
        MAX_RANGE = 10
        for k in range(MAX_RANGE):
                try:
                        k = k + 1
                        sock.settimeout(0.7)
                        sock.sendto(send_str, (args.addr[1], UDP_PORT))
                        i=0
                        data=""
                        while (not expect_str in data and i < 100):
                                i = i + 1
                                sock.settimeout(0.7)
                                data, addr = sock.recvfrom(1024)
                                #print data
                        break
                except:
                        pass
        if k not in range(MAX_RANGE):
                print k
                raise Exception("Communication Error")
        return data


try:
        print "\nTEST LEDs\n"
        tx_expect("set:rled,val:1", "LED_R_ON")
        time.sleep(0.5)
        tx_expect("set:rled,val:0", "LED_R_OFF")
        tx_expect("set:gled,val:1", "LED_G_ON")
        time.sleep(0.5)
        tx_expect("set:gled,val:0", "LED_G_OFF")
        tx_expect("set:bled,val:1", "LED_B_ON")
        time.sleep(0.5)
        tx_expect("set:bled,val:0", "LED_B_OFF")

        print "TEST hdc1000"
        time.sleep(0.1)
        print tx_expect("get:hdc1000", "temp:")

        print "TEST mpl3115a2"
        time.sleep(0.1)
        print tx_expect("get:mpl3115a2", "pressure:")

        print "TEST mma8652"
        time.sleep(0.1)
        print tx_expect("get:mma8652", "x:")

        print "TEST mag3110"
        time.sleep(0.1)
        print tx_expect("get:mag3110", "x:")

        print "TEST tmp006"
        time.sleep(0.1)
        print tx_expect("get:tmp006", "tamb:")

        print "TEST tcs37727"
        time.sleep(0.1)
        print tx_expect("get:tcs37727", "ct:")

        print "FINISH"

except ValueError as err:
        print(err.args)
