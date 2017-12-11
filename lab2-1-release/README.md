# ComputerArchitectureLab
## Lab for Computer Organization and Architecture
## PKU, 2017 Autumn

compile and run:

$ ./run.sh

compile:

$ g++ Read_Elf.h Read_Elf.cpp Simulation.h Simulation.cpp -o simulator
$ riscv64-unknown-elf-gcc -Wa,-march=rv64g -o matmul matrix.c

run normal form:

$ ./simulator filename

run debug form (support single step check and print register and memory infomation)

$ ./simulator -d filename