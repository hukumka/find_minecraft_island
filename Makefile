CC = gcc
FLAGS = -O3 -lm
OBJS = main.o finder.o filters.o

ifeq ($(OS),Windows_NT)
	RM = del
endif

all: cubiomes/libcubiomes.a $(OBJS)
	$(CC) $(FLAGS) $(OBJS) ./cubiomes/libcubiomes.a -o find_island

main.o: finder.h filters.h
finder.o: finder.h
filters.o: finder.h filters.h

clean:
	make -C cubiomes clean
	rm find_island main.o finder.o

cubiomes/libcubiomes.a:
	make -C cubiomes libcubiomes