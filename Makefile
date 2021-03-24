.PHONY: all

#CCFLAGS=-Werror -Wall -Wpedantic -std=c99

all: rf32.fsmod 

rf32.fsmod: 
	gcc src/rf32.FSModifier.c -o Build/rf32.fsmod $(CCFLAGS)

test: rf32.fsmod
	sh -c "cd ../BootFox32/ && make"
	./Build/rf32.fsmod -f ../BootFox32/Floppy.img -a Makefile
