ifneq ($(KERNELRELEASE),)
	obj-m := main2.o
else
	CURRENT = $(shell uname -r)
	KDIR = /lib/modules/$(CURRENT)/build
	PWD = $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	make clean

clean:
	@rm -f *.o .*.cmd .*.flags *.mod.c *.order
	@rm -f .*.*.cmd *~ *.*~ TODO.*
	@rm -fR .tmp*
	@rm -rf .tmp_versions

disclean: clean
	@rm *.ko *.symvers
endif 

load:
	sudo insmod main2.ko
mod:
	lsmod 
log:
	sudo dmesg
remove:
	sudo rmmod main2.ko