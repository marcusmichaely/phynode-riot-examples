# name of your application
APPLICATION = iotk-udp

# If no BOARD is found in the environment, use this default:
BOARD ?= pba-d-01-kw2x

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netif_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules for socket communication via UDP
#USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gnrc_udp
USEMODULE += gnrc_conn_udp
USEMODULE += posix_sockets
USEMODULE += gnrc_icmpv6_echo
# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps

FEATURES_REQUIRED = periph_i2c

USEMODULE += xtimer
USEMODULE += hdc1000
USEMODULE += tmp006
USEMODULE += mpl3115a2
USEMODULE += mag3110
USEMODULE += mma8652
USEMODULE += mma8652
USEMODULE += tcs37727

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include
