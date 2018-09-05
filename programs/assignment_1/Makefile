all: sysinfo

sysinfo: sys_info.o
	gcc sys_info.o -o sys_info

sys_info.o : sys_info.c
	gcc -c sys_info.c

clean: 
	rm -rf *o sys_info
