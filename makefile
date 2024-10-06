CC = gcc
CFLAGS = -g -ansi -pedantic -Wall


# Define the object files
OBJS = general_lib.o assembler.o pre_process.o first_pass.o second_pass.o

# Default target
all: assembler

# Linking step to create the final executable
assembler: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o assembler

# Compilation rules for individual source files
general_lib.o: general_lib.c general_lib.h
	$(CC) $(CFLAGS) -c general_lib.c -o general_lib.o

assembler.o: assembler.c assembler.h
	$(CC) $(CFLAGS) -c assembler.c -o assembler.o

pre_process.o: pre_process.c pre_process.h
	$(CC) $(CFLAGS) -c pre_process.c -o pre_process.o

first_pass.o: first_pass.c first_pass.h
	$(CC) $(CFLAGS) -c first_pass.c -o first_pass.o

second_pass.o: second_pass.c second_pass.h
	$(CC) $(CFLAGS) -c second_pass.c -o second_pass.o
	
