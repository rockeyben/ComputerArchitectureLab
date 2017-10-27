#! /bin/bash

g++ Read_Elf.h Read_Elf.cpp Simulation.h Simulation.cpp -o simulator
riscv64-unknown-elf-gcc -Wa,-march=rv64g -o matmul matrix.c
./simulator matmul
