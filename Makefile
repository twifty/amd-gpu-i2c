CONFIG_MODULE_SIG=n
CONFIG_STACK_VALIDATION=n
MODULE_NAME = amdgpu-i2c

SRCS = \
	amd-gpu-dce.c \
	amd-gpu-dcn.c \
	amd-gpu-reg.c \
	amd-gpu-i2c.c \
	amd-gpu-pci.c \
	main.c

KERNELDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
# KBUILD_EXTRA_SYMBOLS := $(ADAPTERDIR)/Module.symvers
OBJS = $(SRCS:.c=.o)
ccflags-y := -g -Wimplicit-fallthrough=3 -DDEBUG -I$(PWD)/../

ifeq ($(KERNELRELEASE),)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

uninstall:
	sudo rmmod $(MODULE_NAME) || true

install: uninstall all
	sudo insmod $(MODULE_NAME).ko

build: clean all

.PHONY: all clean uninstall install

else

	obj-m += $(MODULE_NAME).o
	$(MODULE_NAME)-y = $(OBJS)

endif
