CC = gcc
FLAGS = -g -O2 -lm -I. -L.
OBJS = main.o finder.o filters.o ocl_generator.o
OCL_OBJS = ocl_tests.o ocl_generator.o cubiomes/libcubiomes.a
override CFLAGS += $(FLAGS)

ifeq ($(OS),Windows_NT)
	RM = del
endif

ocl: $(OCL_OBJS)
	$(CC) $(CFLAGS) $(OCL_OBJS) -o ocl_test -lOpenCL

ocl_tests.o: ocl_generator.h
ocl_generator.o: ocl_generator.h

all: cubiomes/libcubiomes.a $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) ./cubiomes/libcubiomes.a -lOpenCL -o find_island

main.o: finder.h filters.h ocl_generator.h
finder.o: finder.h
filters.o: finder.h filters.h

clean:
	make -C cubiomes clean
	rm find_island main.o finder.o

cubiomes/libcubiomes.a:
	make -C cubiomes libcubiomes