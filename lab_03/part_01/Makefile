CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)
TARGET1 = md
obj-m := $(TARGET1).o

default:
# -C - смена своего каталога на указанный опцией (т.е на каталог исходных кодов ядра)
# там находит Makefile верхнего уровня ядра
# M= заставляет вернуться обратно в директорию исходников модуля,
# прежде чем попытаться построить целевой модуль
# ссылается на список модулей в obj-m
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	@rm -f *.o .*.cmd .*.flags *.mod.c *.order Module.*
	@rm -f .*.*.cmd *~ *.*~ TODO.*
	@rm -fR .tmp*
	@rm -rf .tmp_versions

disclean: clean
	@rm *.ko *.symvers *.mod

load:
	sudo insmod md.ko

remove:
	sudo rmmod md.ko

info:
	modinfo md.ko

logs:
	dmesg