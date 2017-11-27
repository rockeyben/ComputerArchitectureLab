CC=g++

all: sim

sim: Simulation.o cache.o memory.o Read_Elf.o
	$(CC) -o $@ $^

Read_Elf.o: Read_Elf.h

Simulation.o: cache.h memory.h Simulation.h Read_Elf.h Reg_def.h

cache.o: cache.h def.h

memory.o: memory.h

.PHONY: clean

clean:
	rm -rf sim *.o
