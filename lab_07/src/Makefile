CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build 
G = -I /usr/include/x86_64-linux-gnu
PWD = $(shell pwd)
TARGET = slab
obj-m := $(TARGET).o

default:
	$(MAKE) -C $(KDIR) $(G) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd .*.flags *.mod.c *.order Module.*
	@rm -f .*.*.cmd *~ *.*~ TODO.*
	@rm -fR .tmp*
	@rm -rf .tmp_versions
	@rm *.ko *.mod

disclean: clean
	@rm *.ko *.symvers *.mod

load:
	sudo insmod ${TARGET}.ko

remove:
	sudo rmmod ${TARGET}.ko

info:
	modinfo ${TARGET}.ko

logs:
	dmesg

update:
	make clean && make remove && make && make load	