include ../Makefile.include

all: test.run

test.riscv:  test.o kmeans.o barrier.o $(SPMD_COMMON_OBJECTS) ../common/crt.o
	$(RISCV_LINK) $< kmeans.o barrier.o -o $@ $(RISCV_LINK_OPTS)

clean:
	-rm *.o *.jou *.log *.pb bsg_rom_test.v test.riscv *.wdb test.bin test.hex
	-rm -rf xsim.dir











