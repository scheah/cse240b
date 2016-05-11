export RISCV:=$(PWD)/riscv-install
export SED=sed


installs:
	sudo apt-get install autoconf automake libtool curl gawk bison flex texinfo gperf sed autotools-dev libmpc-dev libmpfr-dev libgmp-dev build-essential

checkout-all:
	git clone https://github.com/ucb-bar/rocket-chip.git
	cd rocket-chip; git checkout 48170fd9aa4de6bf2a443010881d25dd55238941; git submodule update --init
	cd rocket-chip/riscv-tools;     git submodule update --init --recursive	

build-riscv-tools:
	cd rocket-chip/riscv-tools; sed -i 's/JOBS=16/JOBS=2/' build.common
	cd rocket-chip/riscv-tools; sed -i 's/build_project riscv-gnu-toolchain/build_project riscv-gnu-toolchain --enable-multilib/' build.sh
	cd rocket-chip/riscv-tools; ./build.sh | tee $@.log

clean:
	rm -rf rocket-chip
