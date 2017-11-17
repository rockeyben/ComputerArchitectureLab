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
#define F7_SUBW 32


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
#define ALU_MOD 12

#define ALU_SHIFTL 5
#define ALU_SHIFTR_L 6
#define ALU_SHIFTR_A 10

#define ALU_AND 7
#define ALU_OR 8
#define ALU_XOR 9
#define ALU_SLT 11

#define ALU_ADDIW 13
#define ALU_BEQ 14
#define ALU_BNE 15
#define ALU_BLT 16
#define ALU_BGE 17

#define ALU_OP1 18
#define ALU_OP2 19

#define ALU_ADDW 20
#define ALU_SUBW 21
#define ALU_MULW 22
#define ALU_DIVW 23
#define ALU_SLLIW 24

#define ll long long


#define MAX 100000000

//主存
unsigned int memory[MAX]={0};
//寄存器堆
REG reg[32]={0};
//PC
ll RS2=0;
ll ALUOUT=0;
ll WBData=0;
int ALU_OP = 0;
ll PC;
unsigned int IR;
ll OP1;
ll OP2;
unsigned int G_rd=0, G_rs1, G_rs2;


//各个指令解析段
unsigned int OP=0;
unsigned int fuc3=0,fuc7=0;
unsigned int rs1=0, rs2=0, rd=0;
unsigned int s_imm=0,i_imm=0,u_imm=0,uj_imm=0, sb_imm=0;
ll s_imm_ext=0, i_imm_ext=0, u_imm_ext=0, uj_imm_ext=0, sb_imm_ext=0;

int ctrl_W_WriteReg;
int ctrl_F_Jump;
int ctrl_M_MemWrite;
int ctrl_M_MemRead;
int nonConditionJump;
int ConditionJump;


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
	unsigned ll one = 1;
	if(sign_bit == 1)
	{
		unsigned ll mask = ~((one << (bit+1)) - 1);
		unsigned ll res = t | mask;
		return res;
	}
	else
	{
		return t;
	}
}

/* general definition of Multiple Selection Device */
class Mux
{
public:
	int input_size;
	int signal;
	ll output;
	int mux_id;
	ll * input = NULL;
	Mux()
	{
		input_size = 0;
		input = NULL;
		signal = 0;
		output = 0;
	}

	Mux(int in_size, int id)
	{
		mux_id = id;
		input_size = in_size;
		input = (ll*)malloc(in_size*64);
		output = 0;
		signal = 0;
	}

	void set_signal(int a)
	{
		signal = a;
	}

	void set_input(ll val, int pos)
	{
		if(pos >= input_size || pos < 0)
		{
			printf("invalid set mux! mux: %d\n", mux_id);
			return ;
		}
		input[pos] = val;
	}

	ll run()
	{
		int o_signal = signal;
		signal = 0;
		return input[o_signal];
	}
}pc_sel, op1_sel, op2_sel, alu_sel, wb_sel;

class CtrlReg
{
public:
	// registers
	ll PC;
	unsigned int IR;
	ll RS2;
	ll OP1;
	ll OP2;
	ll ALUOUT;
	ll WBData;
	unsigned int ALU_OP;
	unsigned int rd;
	unsigned int rs1;
	unsigned int rs2;
	int ctrl_W_WriteReg;
	int ctrl_F_Jump;
	int ctrl_M_MemWrite;
	int ctrl_M_MemRead;

	int pc_signal;
	int alu_signal;

	int stall;

	int nonConditionJump;
	int ConditionJump;

	CtrlReg()
	{
		ALU_OP = 0;
		PC = RS2 = OP1 = OP2 = ALUOUT = WBData = 0;
		IR = 0;
		rd = rs1 = rs2 = -1;
		ctrl_M_MemWrite = ctrl_W_WriteReg = ctrl_F_Jump = ctrl_M_MemRead = 0;
		pc_signal = alu_signal = 0;
		stall = 0;
		nonConditionJump = ConditionJump = 0;
	}

	void clearReg()
	{		
		ALU_OP = 0;
		PC = RS2 = OP1 = OP2 = ALUOUT = WBData = 0;
		IR = 0;
		rd = rs1 = rs2 = -1;
		ctrl_M_MemWrite = ctrl_W_WriteReg = ctrl_F_Jump = ctrl_M_MemRead = 0;
		pc_signal = alu_signal = 0;
		stall = 0;
		nonConditionJump = ConditionJump = 0;
	}
}D_REG, E_REG, M_REG, W_REG, F_REG;
