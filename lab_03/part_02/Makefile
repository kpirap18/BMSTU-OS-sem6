obj-m += md1.o md2.o md3.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build

all default: modules
install: modules_install

modules modules_install help clean:
	$(MAKE) -C $(KERNELDIR) M=$(shell pwd) $@

clean: 
	@rm -f *.o .*.cmd .*.flags *.mod.c *.order
	@rm -f .*.*.cmd *~ *.*~ TODO.* 
	@rm -fR .tmp* 
	@rm -rf .tmp_versions 

disclean: clean 
	@rm *.ko *.symvers *.mod

load_b:
	sudo insmod md1.ko
	sudo insmod md2.ko
	sudo insmod md3.ko

remove_b:
	sudo rmmod md2.ko
	sudo rmmod md1.ko


what_m:
	lsmod | grep md