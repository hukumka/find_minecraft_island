CC = gcc
FLAGS = -O3 -lm

ifeq ($(OS),Windows_NT)
	RM = del
else
	RM = rm
endif

all: cubiomes/libcubiomes.a
	$(CC) $(FLAGS) main.c ./cubiomes/libcubiomes.a -o find_island

clean:
	cd cubiomes
	make clean
	cd ..
	$(RM) find_island

cubiomes/libcubiomes.a:
	cd cubiomes
	make libcubiomes
	cd ..