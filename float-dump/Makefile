RISCV_GCC = ../riscv-tools/riscv-install/bin/riscv64-unknown-elf-gcc
RISCV_OBJDUMP = ../riscv-tools/riscv-install/bin/riscv64-unknown-elf-objdump

dump: float-test.o 
	$(RISCV_OBJDUMP) -d float-test.o > float-test.dump

float-test.o:
	$(RISCV_GCC) float-test.c -c -o float-test.o 
clean: 
	rm *.o *.dump	
