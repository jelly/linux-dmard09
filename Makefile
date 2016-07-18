CROSS_COMPILE=arm-none-eabi-
KDIR=/mnt/msata/linux-hans

obj-m += dmard09.o

default:
	make -C ${KDIR} ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} M=$(PWD) modules
