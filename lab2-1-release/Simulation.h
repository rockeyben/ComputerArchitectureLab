#include <iostream>
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include "Reg_def.h"

#define OP_JAL 111
#define OP_R 51

#define F3_ADD 0
#define F3_MUL 0

#define F7_MSE 1
#define F7_ADD 0

#define OP_I 19
#define F3_ADDI 0

#define OP_SW 35
#define F3_SB 0

#define OP_LW 3
#define F3_LB 0

#define OP_BEQ 99
#define F3_BEQ 0

#define OP_IW 27
#define F3_ADDIW 0

#define OP_RW 59
#define F3_ADDW 0
#define F7_ADDW 0


#define OP_SCALL 115
#define F3_SCALL 0
#define F7_SCALL 0

#define OP_LUI 55
#define OP_AUIPC 23
#define OP_JALR 103

#define ALU_ADD 0
#define ALU_SUB 1
#define ALU_MUL 2
#define ALU_MULH 3
#define ALU_DIV 4
#define ALU_SHIFTL 5
#define ALU_SHIFTR 6
#define ALU_AND 7
#define ALU_OR 8
#define ALU_XOR 9



#define ll long long


#define MAX 100000000

//主存
unsigned int memory[MAX]={0};
//寄存器堆
REG reg[32]={0};
//PC
ll PC=0;


//各个指令解析段
unsigned int OP=0;
unsigned int fuc3=0,fuc7=0;
int shamt=0;
int rs=0,rt=0,rd=0;
unsigned int imm12=0;
unsigned int imm20=0;
unsigned int imm7=0;
unsigned int imm5=0;


//加载内存
void debug();

void load_memory();

void simulate();

void IF();

void ID();

void EX();

void MEM();

void WB();


//符号扩展
int ext_signed(unsigned int src,int bit);

//获取指定位
unsigned int getbit(int s,int e);

unsigned int getbit(unsigned int inst,int s,int e)
{
	unsigned int mask1 = 0xffffffff;
	unsigned int mask2 = 0xffffffff;
	if(s > 0)
	{
		mask1 =  ~((1 << (s-1)) - 1);
	}

	if(e < 31)
	{
		mask2 =  (1 << (e+1)) - 1;
	}
	if(e == 31)
	{
		//printf("get op : %x %x %x %x\n",inst, inst & mask1 & mask2, mask1, mask2);
	}
	unsigned res = inst & mask1 & mask2;
	res = res >> s;
	return res;
}

unsigned int getRs1(unsigned int inst)
{
	return getbit(inst, 15, 19);
}

unsigned int getRs2(unsigned int inst)
{
	return getbit(inst, 20, 24);
}

/* bit represent the position of the highest bit */
int ext_signed(unsigned int src,int bit)
{
	unsigned int sign_bit = (src >> bit) & 1;
	if(sign_bit == 1)
	{
		unsigned int mask = ~((1 << (bit+1)) - 1);
		unsigned int res = src | mask;
		return res;
	}
	else
	{
		return src;
	}
}

ll ext_signed_ll(unsigned int src, int bit)
{
	unsigned int sign_bit = (src >> bit) & 1;
	unsigned ll t = (unsigned ll)src;
	if(sign_bit == 1)
	{
		unsigned ll mask = ~((1 << (bit+1)) - 1);
		unsigned ll res = t | mask;
		return res;
	}
	else
	{
		return t;
	}
}
