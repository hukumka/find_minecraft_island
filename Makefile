CC = gcc
FLAGS = -O3 -lm

ifeq ($(OS),Windows_NT)
	RM = del
endif

all: cubiomes/libcubiomes.a main.o finder.o
	$(CC) $(FLAGS) main.o finder.o ./cubiomes/libcubiomes.a -o find_island

main.o: finder.h
finder.o: finder.h

clean:
	make -C cubiomes clean
	rm find_island main.o finder.o

cubiomes/libcubiomes.a:
	make -C cubiomes libcubiomes