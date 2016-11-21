CROSS_COMP=arm-linux-gnueabihf-
ARM_PATH=/usr/local/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin

ifeq ($(ARCH),arm)
GCC=${ARM_PATH}/${CROSS_COMP}gcc
else
GCC=gcc
endif
memtool: memtool.c
	${GCC} $^ -g -ggdb -o $@
Devmem2: Devmem2.c
	${GCC} $^ -o $@
clean:
	rm -rf *.o memtool
