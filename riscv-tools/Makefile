export RISCV:=$(PWD)/riscv-install
export SED=sed


installs:
	sudo apt-get install autoconf automake libtool curl gawk bison flex texinfo gperf sed autotools-dev libmpc-dev libmpfr-dev libgmp-dev build-essential

checkout-all:
	git clone https://github.com/ucb-bar/rocket-chip.git
	cd rocket-chip; git submodule update --init
	cd rocket-chip/riscv-tools;     git submodule update --init --recursive	

build-riscv-tools:
	cd rocket-chip/riscv-tools; sed -i 's/JOBS=16/JOBS=2/' build.common
	cd rocket-chip/riscv-tools; ./build.sh | tee $@.log

build-riscv32:
	mkdir -p rocket-chip/riscv-tools/riscv-gnu-toolchain/build32
	cd rocket-chip/riscv-tools/riscv-gnu-toolchain/build32; ../configure --with-xlen=32 --with-arch=IM --prefix=$(RISCV); make


clean:
	rm -rf rocket-chip
