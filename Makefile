all: libharden.so

libharden.so: harden-opendir.c
	gcc -shared -fpic -std=c99 -o $@ $<
