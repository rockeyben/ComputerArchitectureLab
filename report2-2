# 计算机组织与系统结构实习报告 Lab 2.2
学号：1500012752
姓名：薛犇
大班老师：程旭

## Part I ：RISCV多周期模拟器
1 .  指令各阶段的寄存器传输级描述

|信息|值|
|---|---:|
PC| 指令地址寄存器
IR| 指令寄存器
RS2| rs2寄存器中的值
OP1| 传入ALU的第一个参数
OP2| 传入ALU的第二个参数
WBData| 写回寄存器的值
ALUOUT| 执行阶段ALU的输出
ALU_OP| 执行阶段ALU执行的操作
rs1| rs1寄存器编码
rs2| rs2寄存器编码
rd| rd寄存器编码
ctrl_W_WriteReg|写回阶段是否写入寄存器
ctrl_F_Jump|取址阶段是否跳转
ctrl_M_MemWrite|访存阶段是否写入内存
ctrl_M_MemRead|访存阶段是否读取内存
stall|本阶段是否暂停
nonConditionJump|本指令是否为无条件跳转
ConditionJump|本指令是否为条件跳转

另外设置了多选器**pc_sel**
有三个输入：**PC+4**, **br**, **jalr**，分别对应下一条指令、sb跳转指令、uj型跳转指令的地址，在每个取址阶段PC寄存器会从这三个输入中选择一个，别的阶段中不能修改PC寄存器，只能修改多选器**pc_sel**的输入。

设置了多选器**alu_sel**
有两个输入：**PC**, **ALUOUT**，分别对应PC寄存器的值和执行阶段的结果。


- R 型指令 （以 add 为例）
```
add rd, rs1, rs2
  共分为4个阶段
  取值：IR = imem[PC], PC = PC + 4
  译码：OP = 0x3, fuc3 = 0x0, fuc7 = 0x00, rs1 = IR[15:19], rs2 = IR[20:24], rd = IR[7, 11], OP1 = reg[rs1], OP2 = reg[rs2] 
  执行：ALUOUT = OP1 + OP2
  写回：ctrl_W_WriteReg = 1, WBData = ALUOUT

```

- I 型指令（以 addi 为例）
```
addi rd, rs1, imm
  共分为4个阶段
  取值：IR = imem[PC], PC = PC + 4
  译码：OP = 0x13, fuc3 = 0x0, rs1 = IR[15:19], rd = IR[7, 11], imm = IR[20:31], OP1 = reg[rs1], OP2 = imm 
  执行：ALUOUT = OP1 + OP2
  写回：ctrl_W_WriteReg = 1, WBData = ALUOUT
```

- I型指令（以 lw 为例）
```
lw rd, offset(rs1)
  共分为5个阶段
  取值：IR = imem[PC], PC = PC + 1
  译码：OP = 0x03, fuc3 = 0x0, rs1 = IR[15:19], rd = IR[7, 11], imm = IR[20:31], OP1 = reg[rs1], OP2 = imm 
  执行：ALUOUT = OP1 + OP2
  访存：rdata = dmem[ALUOUT]
  写回：ctrl_W_WriteReg = 1, WBData = rdata
```

- I型指令（以 jalr 为例）
```
jalr rd, rs1, imm
  共分为4个阶段
  取值：IR = imem[PC], PC = PC + 4
  译码：OP = 0x67, fuc3 = 0x0, rs1 = IR[15:19], rd = IR[7, 11], imm = IR[20:31], OP1 = reg[rs1], OP2 = imm 
  执行：ALUOUT = PC  + 4, PC = OP1 + OP2
  写回：ctrl_W_WriteReg = 1, wdata = ALUOUT
```

- S型指令（以 sw 为例）
```
sw rs2, offset(rs1)
  共分为4个阶段
  取值：IR = imem[PC], PC = PC + 4
  译码：OP = 0x23, fuc3 = 0x2, rs1 = IR[15:19], rs2 = IR[20, 24], imm = IR[7,11]|IR[25,31]<<5, OP1 = reg[rs1], OP2 = imm 
  执行：ALUOUT = OP1 + OP2
  访存：ctrl_M_MemWrite= 1, wdata = reg[rs2], waddr = ALUOUT
```
- SB型指令 (以 beq 为例)
```
beq rs1, rs2, offset
  共分为3个阶段
  取值：IR = imem[PC], PC = PC + 4
  译码：OP = 0x63, fuc3 = 0x0, rs1 = IR[15:19], rs2 = IR[20, 24], imm = IR[7]<<11|IR[8,11]<<1|IR[25,30]<<5|IR[31]<<12, OP1 = reg[rs1], OP2 = imm 
  执行：if OP1 == reg[rs2]:
  			PC = PC + OP2
```

- U 型指令（以 lui 为例）
```
lui rd, offset
  共分为4个阶段
  取值：IR = imem[PC], PC = PC + 4
  译码：OP = 0x37, rd = IR[7,11], imm = IR[12:31]<<20, OP2 = imm 
  执行：ALUOUT = imm
  写回：ctrl_W_WriteReg= 1, wdata = ALUOUT
```
- UJ型指令（以 jal 为例）
```
jal rd, imm
  共分为4个阶段
  取值：IR = imem[PC], PC = PC + 4
  译码：OP = 0x6f, rd = IR[7,11], imm = IR[12:19]<<12|IR[20,20]<<11|IR[21,30]<<1|IR[31]<<20, OP2 = imm 
  执行：new_PC = PC + imm
  写回：ctrl_W_WriteReg = 1, wdata = old_PC + 1
```

2 . 数据通路图：
控制信号产生逻辑：
```
// if jalr or jal, set pc and alu input
if(OP==0x67||OP==0x6f)
{
	alu_sel.set_signal(1);
	pc_sel.set_signal(2);
}
// set load length, lb, lh, lw, ld
if(OP==0x03)
{
	ctrl_M_MemRead = fuc3 + 1;
}

// set store length, sb, sh, sw, sd
if(OP==0x23)
{
	ctrl_M_MemWrite = fuc3 + 1;
}

// non conditional jump
if(OP==0x67 || OP==0x6f)
	nonConditionJump = 1;

// conditional jump
if(OP==0x63)
	ConditionJump = 1;
	
// need to write registers
if(OP!=0x23 && OP!=0x63)
	ctrl_W_WriteReg = 1;

//  if conditional jump happens
if(COND_J==1)
{
	pc_sel.set_signal(1);
	pc_sel.set_input(PC + OP2, 1);
}
```
3 . 运行结果

| Progs        | inst nums         | cycles  |CPI|
| ------------- |:-------------:|-----:| ----|
| test1      | 29 | 126 | 4.34 |
| test2      | 29 | 126 | 4.34 |
| test3      | 118 | 498 | 4.22|
| test4      | 163 | 693 | 4.25|
|test5       |119|697|5.86|
|test6|69|487|7.06|
|test7|133|563|4.23|
|test8|61|255|4.18|
|test9|34|186|5.47|
|test10|24|144|6.00|

## Part II : RISCV 流水线模拟器

1 .  指令各阶段的寄存器传输级描述

注意：设置了5个临时寄存器堆，对应存储5个阶段的一些控制信号和变量信息。
分别是：**F_REG**,**D_REG**, **E_REG**, **M_REG**, **W_REG**，每个周期结束后，这几个寄存器的值会依次向后拷贝，例如D_REG的传入E_REG中，E_REG传入M_REG中，而F_REG的更新则由pc_sel多选器决定。如果某个阶段的stall值为1,那么这个阶段的寄存器不向后拷贝。若需要bubble，则清空这个阶段的寄存器堆。

存储了以下信息：

|信息|值|
|---|---:|
PC| 指令地址寄存器
IR| 指令寄存器
RS2| rs2寄存器中的值
OP1| 传入ALU的第一个参数
OP2| 传入ALU的第二个参数
WBData| 写回寄存器的值
ALUOUT| 执行阶段ALU的输出
ALU_OP| 执行阶段ALU执行的操作
rs1| rs1寄存器编码
rs2| rs2寄存器编码
rd| rd寄存器编码
ctrl_W_WriteReg|写回阶段是否写入寄存器
ctrl_F_Jump|取址阶段是否跳转
ctrl_M_MemWrite|访存阶段是否写入内存
ctrl_M_MemRead|访存阶段是否读取内存
stall|本阶段是否暂停
nonConditionJump|本指令是否为无条件跳转
ConditionJump|本指令是否为条件跳转

多选器设置同Part1

- R 型指令 （以 add 为例）
```
add rd, rs1, rs2
  共分为4个阶段
  取值：F_REG.IR = imem[F_REG.PC], pc_sel.input[0] = F_REG.PC + 4
  译码：OP = 0x3, fuc3 = 0x0, fuc7 = 0x00, D_REG.rs1 = IR[15:19], D_REG.rs2 = IR[20:24], D_REG.rd = IR[7, 11], D_REG.OP1 = reg[rs1], D_REG.OP2 = reg[rs2] 
  执行：E_REG.ALUOUT = E_REG.OP1 + E_REG.OP2
  写回：W_REG.ctrl_W_WriteReg = 1, W_REG.WBData = W_REG.ALUOUT

```

- I 型指令（以 addi 为例）
```
addi rd, rs1, imm
  共分为4个阶段
  取值：F_REG.IR = imem[PC], pc_sel.input[0] = F_REG.PC + 4 
  译码：OP = 0x13, fuc3 = 0x0, D_REG.rs1 = IR[15:19], D_REG.rd = IR[7, 11], imm = IR[20:31], D_REG.OP1 = reg[rs1], D_REG.OP2 = imm 
  执行：E_REG.ALUOUT = E_REG.OP1 + E_REG.OP2
  写回：W_REG.ctrl_W_WriteReg = 1, W_REG.WBData = W_REG.ALUOUT
```

- I型指令（以 lw 为例）
```
lw rd, offset(rs1)
  共分为5个阶段
  取值：F_REG.IR = imem[F_REG.PC], pc_sel.input[0] = F_REG.PC + 4
  译码：OP = 0x03, fuc3 = 0x0, D_REG.rs1 = IR[15:19], D_REG.rd = IR[7, 11], imm = IR[20:31], D_REG.OP1 = reg[rs1], D_REG.OP2 = imm 
  执行：E_REG.ALUOUT = E.REG.OP1 + E_REG.OP2
  访存：M_REG.rdata = dmem[M_REG.ALUOUT]
  写回：W_REG.ctrl_W_WriteReg = 1, W_REG.WBData = rdata
```

- I型指令（以 jalr 为例）
```
jalr rd, rs1, imm
  共分为4个阶段
  取值：F_REG.IR = imem[PC]
  译码：OP = 0x67, fuc3 = 0x0,D_REG. rs1 = IR[15:19], D_REG.rd = IR[7, 11], imm = IR[20:31], D_REG.OP1 = reg[rs1], D_REG.OP2 = imm 
  执行：E_REG.ALUOUT = E_REG.PC  + 4, pc_sel.input[2]= OP1 + OP2
  写回：W_REG.ctrl_W_WriteReg = 1, wdata = W_REG.ALUOUT
```

- S型指令（以 sw 为例）
```
sw rs2, offset(rs1)
  共分为4个阶段
  取值：F_REG.IR = imem[PC], pc_sel.input[0] = F_REG.PC + 4
  译码：OP = 0x23, fuc3 = 0x2, D_REG.rs1 = IR[15:19], D_REG.rs2 = IR[20, 24], imm = IR[7,11]|IR[25,31]<<5, D_REG.OP1 = reg[rs1], D_REG.OP2 = imm 
  执行：E_REG.ALUOUT = E_REG.OP1 + E_REG.OP2
  访存：M_REG.ctrl_M_MemWrite= 1, wdata = reg[rs2], waddr = E_REG.ALUOUT
```
- SB型指令 (以 beq 为例)
```
beq rs1, rs2, offset
  共分为3个阶段
  取值：F_REG.IR = imem[PC], pc_sel.input[0] = F_REG.PC + 4
  译码：OP = 0x63, fuc3 = 0x0, D_REG.rs1 = IR[15:19], D_REG.rs2 = IR[20, 24], imm = IR[7]<<11|IR[8,11]<<1|IR[25,30]<<5|IR[31]<<12, D_REG.OP1 = reg[rs1], D_REG.OP2 = imm 
  执行：if E_REG.OP1 == E_REG.RS2:
  			pc_sel.input[1] = E_REG.PC + E_REG.OP2
```

- U 型指令（以 lui 为例）
```
lui rd, offset
  共分为4个阶段
  取值：F_REG.IR = imem[PC], pc_sel.input[0] = F_REG.PC + 4
  译码：OP = 0x37, D_REG.rd = IR[7,11], imm = IR[12:31]<<20, D_REG.OP2 = imm 
  执行：E_REG.ALUOUT = imm
  写回：W_REG.ctrl_W_WriteReg= 1, wdata = W_REG.ALUOUT
```
- UJ型指令（以 jal 为例）
```
jal rd, imm
  共分为4个阶段
  取值：F_REG.IR = imem[PC], pc_sel.input[0] = F_REG.PC + 4
  译码：OP = 0x6f, D_REG.rd = IR[7,11], imm = IR[12:19]<<12|IR[20,20]<<11|IR[21,30]<<1|IR[31]<<20, D_REG.OP2 = imm 
  执行：pc_sel.input[2] = E_REG.PC + E_REG.OP2
  写回：W_REG.ctrl_W_WriteReg = 1, wdata = W_REG.PC + 1
```

2 . 数据通路图
控制信号逻辑：
```
// set load length, lb, lh, lw, ld
if(OP==0x03)
{
	D_REG.ctrl_M_MemRead = fuc3 + 1;
}

// set store length, sb, sh, sw, sd
if(OP==0x23)
{
	D_REG.ctrl_M_MemWrite = fuc3 + 1;
}

// non conditional jump
if(OP==0x67 || OP==0x6f)
	D_REG.nonConditionJump = 1;

// conditional jump
if(OP==0x63)
	D_REG.ConditionJump = 1;

// need to write Registers
if(OP!=0x23 && OP!=0x63)
	D_REG.ctrl_W_WriteReg = 1;

// if conditional jump happens
if(COND_J==1)
{
	// set pc input
	E_REG.pc_signal = 1; 
	pc_sel.set_input(E_REG.PC + OP2, 1);
	pc_sel.set_signal(1);
	// set jump signal
	E_REG.ctrl_F_Jump = 1;
}

// if jal or jalr, set pc input and alu input
if(OP==0x67||OP==0x6f)
{
	D_REG.alu_signal = 1;
	D_REG.pc_signal = 2;
}
```

3 . 冒险分析

- 数据冒险
当decode阶段的src寄存器与execute, memory, writeback阶段的dst寄存器相同时，产生数据冒险
```
add a4, a5, a0
sw a4, (s0)
```
sw用到的a4寄存器在add的writeback阶段才会被写回，所以会产生冒险。采用stall，暂停sw指令，直到add指令执行完成。

- 分支预测冒险
当遇到条件分支指令时，只有在execute阶段结束吼才知道是否跳转
```
beq a4, a5, offset
addi a4, a5, 0
subi a5, a4, 0
```
采用静态预测分支，例如beq下面的两条指令，默认执行，如果发现需要跳转，那么bubble掉两条指令。


4 . 运行结果

| Progs        | inst nums         | cycles  |CPI|
| ------------- |:-------------:|-----:| ----|
| test1      | 29 | 71 | 2.45 |
| test2      | 29 |  71 | 2.45|
| test3      | 119 | 292 | 2.45|
| test4      | 164 | 412 | 2.51|
|test5       |120|491|4.09|
|test6|70|171|2.44|
|test7|134|337|2.51|
|test8|61|181|2.97|
|test9|34|72|2.12|
|test10|24|54|2.25|

5 . stall分析

| Progs        | data stall      | contol bubble|
| ------------- |:-------------:|-----:|
| test1      | 42 | 0| 
|test2|42|0|
|test3|162|11|
|test4|237|11|
|test5|165|11|
|test6|90|11|
|test7|192|11|
|test8|120|0|
|test9|34|4|
|test10|30|0|

分析：
（1）所有的程序相较于多周期处理器，CPI显著下降
（2）可以看出，有循环的程序普遍存在利用bubble解决控制冒险的现象。
（3）从程序7和程序8的执行中也可以看出：程序8因为没有使用循环，所以避免了很多控制冒险，并且排除了循环外指令对循环内指令的干扰，因为程序7明显有循环外指令产生数据冒险增加stall次数的现象。这表明，合理机智地安排指令顺序，或者编译代码，能够提升不少程序性能。










