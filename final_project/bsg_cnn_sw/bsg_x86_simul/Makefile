BSG_LIB_EXT_PATH=../bsg_lib_ext
TARGET=token_test

all:
	gcc -o $(TARGET) token_test.c bsg_util_x86_simul.c -I$(BSG_LIB_EXT_PATH) -lpthread -Dbsg_tiles_X=2 -Dbsg_tiles_Y=1

clean:
	rm -f $(TARGET)

