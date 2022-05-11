## [M1: point 1]
#  Assigning the name of the module
#  ...
MODULE	 = s2fs

## [M2: point 1]
#  Makes the .ko file to install
#  ...
obj-m += $(MODULE).o



## [M3: point 1]
#  Tell what directory to compile in
#  ...
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

## [M4: point 1]
#  Get the current directory path
#  ...
PWD := $(shell pwd)

## [M5: point 1]
#  Set the default target to use the name initialized above
#  ...
all: $(MODULE)  

## [M6: point 1]
#  Set the compiler to output all files compiled
#  ...
%.o: %.c
	@echo "  CC      $<"
	@$(CC) -c $< -o $@

## [M7: point 1]
#  Set the target to make the module and put it in linux modules
#  ...
$(MODULE):
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
  
## [M8: point 1]
#  Set the clean target to clean the specified kernel directory
#  ...
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

#insert module
mount:
	sudo insmod $(MODULE).ko
	sudo mount -t $(MODULE) nodev mnt

remove:
	sudo umount ./mnt
	sudo rmmod s2fs
