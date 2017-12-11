# ComputerArchitectureLab
## Lab2-2 for Computer Organization and Architecture
## PKU, 2017 Autumn

compile:

$ g++ Read_Elf.h Read_Elf.cpp Simulation.h Simulation.cpp -o simulator

run normal form:

$ ./simulator [filename]

run debug form (support single step check and print register and memory infomation)

$ ./simulator -d [filename]