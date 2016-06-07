bsg_tiles_X = 2
bsg_tiles_Y = 2

SOFTLOAT_SOURCE_DIR = ./berkeley-softfloat-3/source/
SOFTFLOAT_BUILD_DIR = ./softfloat_build

include $(SOFTFLOAT_BUILD_DIR)/Makefile.softfloat_def
include ../Makefile.include

all: main.run

main.riscv:  float-test.o $(SOFTFLOAT_ARC_LIB) $(SPMD_COMMON_OBJECTS) ../common/crt.o
	$(RISCV_LINK) $< $(SOFTFLOAT_ARC_LIB) -o $@ $(RISCV_LINK_OPTS) 

$(SOFTFLOAT_ARC_LIB):
	cd $(SOFTFLOAT_BUILD_DIR) && $(MAKE)
	cp $(SOFTFLOAT_BUILD_DIR)/$(SOFTFLOAT_ARC_LIB) .

main.o: Makefile
	 
clean: 
	rm -f *.o *.dump
	#rm -rf $(SOFTFLOAT_ARC_LIB)
	#cd $(SOFTFLOAT_BUILD_DIR) && $(MAKE) clean
