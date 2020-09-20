CC = gcc
FLAGS = -O3 -lm

ifeq ($(OS),Windows_NT)
	RM = del
endif

all: cubiomes/libcubiomes.a
	$(CC) $(FLAGS) main.c ./cubiomes/libcubiomes.a -o find_island

clean:
	make -C cubiomes clean
	rm find_island

cubiomes/libcubiomes.a:
	make -C cubiomes libcubiomes