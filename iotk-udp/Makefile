# name of your application
APPLICATION = iotk-udp

# If no BOARD is found in the environment, use this default:
BOARD ?= pba-d-01-kw2x

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Uncomment this to enable scheduler statistics for ps:
#CFLAGS += -DSCHEDSTATISTICS

# Uncomment this to enable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
export QUIET ?= 1

# Modules to include:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += uart0
USEMODULE += ps
USEMODULE += defaulttransceiver
USEMODULE += udp

FEATURES_REQUIRED = periph_i2c

USEMODULE += vtimer
USEMODULE += hdc1000
USEMODULE += tmp006
USEMODULE += mpl3115a2
USEMODULE += mag3110
USEMODULE += mma8652

include $(RIOTBASE)/Makefile.include