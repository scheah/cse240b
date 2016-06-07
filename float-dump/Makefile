SOFTFLOAT = ./softfloat
SOFTFLOAT_INCLUDES = ./softfloat/include/
SOFTFLOAT_OBJECTS = ./softfloat/objects/*.*

bsg_tiles_X = 4
bsg_tiles_Y = 4

RISCV_GCC_EXTRA_OPTS ?= -O2 -I$(SOFTFLOAT) -I$(SOFTFLOAT_INCLUDES) -I$(SOFTFLOAT_OBJECTS) -I$(BSG_LIB_EXT_PATH)

include ../Makefile.include

all: main.run

main.riscv:  $(SOFTFLOAT_OBJECTS) $(SPMD_COMMON_OBJECTS) ../common/crt.o
	$(RISCV_LINK) $(SOFTFLOAT_OBJECTS) -o $@ $(RISCV_LINK_OPTS)

main.o:
	Makefile
	 
clean: 
	rm *.o *.dump	
