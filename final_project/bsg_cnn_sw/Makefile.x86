BSG_LIB_EXT_PATH=./bsg_lib_ext
BSG_X86_SIMUL_PATH=./bsg_x86_simul

TARGET=main

all:
	gcc -o $(TARGET) bsg_cnn.c sweep_propagate.c layer_common.c conv_layer.c maxpool_layer.c fullcon_layer.c output_layer.c $(BSG_X86_SIMUL_PATH)/bsg_util_x86_simul.c $(BSG_X86_SIMUL_PATH)/x86_weight_loader.c -I$(BSG_X86_SIMUL_PATH) -I$(BSG_LIB_EXT_PATH) -lpthread -lm -Dbsg_tiles_X=4 -Dbsg_tiles_Y=4

clean:
	rm -f $(TARGET)

