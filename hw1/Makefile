BSG_IP_CORES_DIR=bsg_ip_cores
BSG_MANYCORE_DIR=bsg_manycore
BSG_IP_CORES_FILES=bsg_test/bsg_nonsynth_reset_gen.v bsg_test/bsg_nonsynth_clock_gen.v bsg_misc/bsg_cycle_counter.v
BSG_MANYCORE_DIR_FILES=
VERILOG_FILES=design.sv tb_nonsynth.sv $(foreach x,$(BSG_IP_CORES_FILES),$(BSG_IP_CORES_DIR)/$(x))
TOPLEVEL=tb_nonsynth

$(warning VERILOG_FILES=$(VERILOG_FILES))

nothing:

run: $(VERILOG_FILES)
	xvlog -sv $^
	@echo \#\#\# bsg elaborating...
	xelab -debug typical $(TOPLEVEL) -s top_sim
	@echo \#\#\# bsg simulating
	xsim top_sim --runall

setup:
	-git clone https://bitbucket.org/taylor-bsg/bsg_ip_cores.git
	-git clone https://bitbucket.org/taylor-bsg/bsg_manycore.git

clean:
	rm -rf *.log *.jou *.pb xsim.dir *.wdb

dist_clean: clean
	rm -rf bsg_ip_cores bsg_manycore
