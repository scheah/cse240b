all:
	gcc -o main bsg_cnn.c bsg_util_x86_simul.c sweep_propagate.c layer_common.c conv_layer.c maxpool_layer.c fullcon_layer.c output_layer.c x86_weight_loader.c -lpthread -lm -Dbsg_tiles_X=4 -Dbsg_tiles_Y=4

token_test:
	gcc -o token_test token_test.c bsg_util_x86_simul.c -lpthread -Dbsg_tiles_X=2 -Dbsg_tiles_Y=1

clean:
	rm -f main token_test

