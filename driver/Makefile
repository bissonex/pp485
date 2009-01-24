#Dual Purpose Makefile to support kernel module builds
ifneq ($(KERNELRELEASE),)
obj-m       := pp485.o
EXTRA_CFLAGS += -I$(obj)/../include
else
KDIR        := /lib/modules/$(shell uname -r)/build
PWD         := $(shell pwd)

all: default
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules_install

endif
