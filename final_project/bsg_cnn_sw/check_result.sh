cat out_all.ret.txt  | grep 00004444 | awk {'print $9'} | awk '{cmd="printf %d 0x" $1; cmd | getline decimal; close(cmd); print decimal}'
