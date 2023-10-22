# name of the application
APPLICATION = sphinx-networking

# If no BOARD is found in the environment, use this default:
BOARD ?= native

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif
# Specify the mandatory networking modules
USEMODULE += gnrc_udp
USEMODULE += gnrc_ipv6_router_default
USEMODULE += sock_udp
USEMODULE += sock_async_event

# Shell modules
USEMODULE +=  shell
USEMODULE +=  shell_cmds_default
USEMODULE +=  ps

# Crypto packages
USEPKG += tweetnacl


# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

#RIOT edits
CFLAGS += -DCONFIG_GNRC_SOCK_MBOX_SIZE_EXP=4
CFLAGS += -DGNRC_UDP_STACK_SIZE=576


include $(RIOTBASE)/Makefile.include
