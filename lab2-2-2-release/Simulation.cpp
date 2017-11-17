#include "Simulation.h"
#include "stdio.h"
#include "Read_Elf.h"


using namespace std;

bool debug_flag = false;

extern void read_elf(const char*filename);
extern Elf64_Ehdr elf64_hdr;
extern Elf64_Off code_offset;
extern Elf64_Addr code_segment;
extern Elf64_Xword code_size;
extern Elf64_Off data_offset;
extern Elf64_Addr data_segment;
extern Elf64_Xword data_size;
extern Elf64_Addr main_in;
extern Elf64_Addr main_end;
extern Elf64_Off SP_offset;
extern Elf64_Addr SP;
extern Elf64_Xword SP_size;
extern Elf64_Xword main_size;
extern Elf64_Addr gp;
extern char* all_start;
extern Elf64_Addr a_addr;
extern Elf64_Addr b_addr;
extern Elf64_Addr c_addr;
extern Elf64_Addr result_addr;
extern Elf64_Addr sum_addr;
extern Elf64_Addr temp_addr;
//extern void read_elf();
extern FILE *file;

void printOP(unsigned int inst);
//指令运行数
long long inst_num=0;
long long cycles=0;
long long data_stall = 0;
long long control_bubble = 0;
long long penalty_inst_stall = 0;
//系统调用退出指示
int exit_flag=0;

//加载代码段
//初始化PC

unsigned int fetch_ui(char*c)
{
	unsigned int ui = 0;
	for(int i = 0; i < 4; i++)
	{
		unsigned int uic = (unsigned int)c[i];
		uic = uic & ((1<<8)-1);
		int sh = i * 8;
		ui = ui | (uic << sh);
	}
	return ui;
}

void load_memory()
{

	for(int i = 0; i < (code_size >> 2); i++)
	{
		memory[i+(code_segment>>2)] = fetch_ui(all_start+code_offset+i*4);
	}
	for(int i = 0; i < (data_size >> 2); i++)
	{
		memory[i+(data_segment>>2)] = fetch_ui(all_start+data_offset+i*4);
	}
}

int main(int argc, char*argv[])
{	

	if(argc == 3)
	{
		if(!strcmp(argv[1], "-d"))
		{
			debug_flag =true;
			read_elf(argv[2]);
		}
	}
	else if(argc <= 2)
	{
		read_elf(argv[1]);
	}
	
	//加载内存
	load_memory();
	pc_sel = Mux(3, 0);
	op1_sel = Mux(2, 2);
	op2_sel = Mux(6, 1);
	alu_sel = Mux(2, 3);
	wb_sel = Mux(2, 4);
	//设置入口地址
	F_REG.PC=main_in>>2;
	pc_sel.set_input(F_REG.PC, 0);
	
	//设置全局数据段地址寄存器
	reg[3]=gp;
	
	reg[2]=MAX/2;//栈基址 （sp寄存器）
	//debug();
	simulate();

	printf("Final information:\n\n");
	debug();
	printf("data stall:%lld control bubble: %lld\n", data_stall, control_bubble);
	printf("cycles: %lld inst_num: %lld\n", cycles, inst_num);
	printf("CPI: %.2f\n", (float)cycles/(float)inst_num);
	cout <<"simulate over!"<<endl;

	return 0;
}

void debug()
{
	printf("Regs:\n");
	for(int i = 0; i < 32; i++)
		printf("%d. %llx  ", i, reg[i]);
	printf("\n");
	unsigned int g = result_addr>>2;
	
	printf("\nresult: \n");
	for(unsigned int i = 0; i < 9; i++)
	{
		printf("Mem: addr 0x%08x val %d\n", (g+i) << 2, memory[g+i]);
	}
	printf("\n");
	printf("a:    addr 0x%08lx val %d\n", a_addr, memory[a_addr>>2]);
	printf("b:    addr 0x%08lx val %d\n", b_addr, memory[b_addr>>2]);
	printf("c:    addr 0x%08lx val %d\n", c_addr, memory[c_addr>>2]);
	printf("sum:  addr 0x%08lx val %d\n", sum_addr, memory[sum_addr>>2]);
	printf("temp: addr 0x%08lx val %d\n", temp_addr, memory[temp_addr>>2]);
	printf("\n");

}

void simulate()
{
	//结束PC的设置
	long long end=(long long)main_end/4 - 1;
	int DIV_STALL = 0;
	while(F_REG.PC!=end)
	{
		cycles += 1;
		//运行
		IF();
		ID();
		EX();
		MEM();
		WB();

		/* data hazard */
		// The destination reg in E M W is same as the source reg in D.
		// Strategy: stall untill the hazard is handled.
		//rintf("%d %d %d %d %d\n", D_REG.rs1, D_REG.rs2, E_REG.rd, M_REG.rd, W_REG.rd);
		if((D_REG.rs1 == E_REG.rd && D_REG.rs1 != -1) ||
			(D_REG.rs2 == E_REG.rd && D_REG.rs2 != -1) ||
			(D_REG.rs1 == M_REG.rd && D_REG.rs1 != -1) ||
			(D_REG.rs2 == M_REG.rd && D_REG.rs2 != -1) ||
			(D_REG.rs1 == W_REG.rd && D_REG.rs1 != -1) ||
			(D_REG.rs2 == W_REG.rd && D_REG.rs2 != -1))
		{	
			data_stall += 1;
			if(debug_flag)
				printf("data hazard! stall\n");
			D_REG.stall = 1;
			F_REG.stall = 1;
		}
		else
		{
			D_REG.stall = 0;
			F_REG.stall = 0;
			/* penalty instruction */
			// (1) 64 bit mul
			if(E_REG.ALU_OP==ALU_MUL || E_REG.ALU_OP==ALU_MULH)
			{
				penalty_inst_stall += 1;
				if(debug_flag)
					printf("penalty! 64bit mul! stall 1 cycle\n");
				E_REG.stall = 1;
				D_REG.stall = 1;
				F_REG.stall = 1;
			}
			else
			{
				E_REG.stall = 0;
				D_REG.stall = 0;
				F_REG.stall = 0;
			}

			// (2) div

			if((E_REG.ALU_OP==ALU_DIV || E_REG.ALU_OP==ALU_DIVW) && DIV_STALL==0)
			{
				DIV_STALL = 40;
			}

			if(DIV_STALL > 1)
			{
				penalty_inst_stall += 1;
				if(debug_flag)
					printf("penalty! 64bit mul! stall %d cycle\n", DIV_STALL);
				E_REG.stall = 1;
				D_REG.stall = 1;
				F_REG.stall = 1;
				DIV_STALL -= 1;
			}
			else
			{
				E_REG.stall = 0;
				D_REG.stall = 0;
				F_REG.stall = 0;
				DIV_STALL = 0;
			}
		}

		/* control hazard */
		// (1) conditional jumps
		// Don't know jump or not under SB instructions.
		// The result is only known after E.
		// Strategy: defaultly predict the jump will happen,
		// If find fault in E, reset the jump, and bubble the instructions which
		// entered into pipeline after prediction.

		if(E_REG.ConditionJump == 1 && E_REG.ctrl_F_Jump == 1)
		{
			control_bubble += 1;
			if(debug_flag)
				printf("control hazard! bubble E\n");
			inst_num -= 1;
			F_REG.clearReg();
			D_REG.clearReg();
		}

		// (2) non conditional jumps

		if(D_REG.nonConditionJump == 1 || E_REG.nonConditionJump == 1)
		{
			control_bubble += 1;
			if(debug_flag)
				printf("control hazard! nonConditionJump bubble\n");
			F_REG.clearReg();
			inst_num -= 1;
		}





		if(W_REG.stall == 0)
		{
			memcpy(&W_REG, &M_REG, sizeof(CtrlReg));
			W_REG.stall = 0;
		}
		if(M_REG.stall == 0)
		{
			memcpy(&M_REG, &E_REG, sizeof(CtrlReg));
			M_REG.stall = 0;
		}

		if(E_REG.stall == 0)
		{
			pc_sel.set_signal(E_REG.pc_signal);
			memcpy(&E_REG, &D_REG, sizeof(CtrlReg));
			E_REG.stall = 0;
		}

		if(D_REG.stall == 0)
		{
			memcpy(&D_REG, &F_REG, sizeof(CtrlReg));
			D_REG.stall = 0;
		}

		if(D_REG.stall == 1 && E_REG.stall == 0)
		{
			E_REG.clearReg();
		}





        if(exit_flag==1)
            break;

        reg[0]=0;//一直为零
        if(debug_flag)
        {
			debug();
			char q = cin.get();
			if(q=='q')
			{
				exit(0);
			}
        }
    }
}


//取指令
void IF()
{
	//write IF_ID_old
	if(F_REG.stall == 0)
	{
		//printf("signal: %d input: %llx\n", pc_sel.signal, pc_sel.input[pc_sel.signal]);
		inst_num += 1;
		F_REG.PC=pc_sel.run();
		F_REG.IR=memory[F_REG.PC];

		pc_sel.set_input(F_REG.PC+1, 0);
	}
}

//译码
void ID()
{
	//Read IF_ID
	unsigned int inst=D_REG.IR;

	rd=getbit(inst,7,11);
	rs1=getbit(inst, 15, 19);
	rs2=getbit(inst, 20, 24);

	fuc3=getbit(inst,12,14);
	OP=getbit(inst, 0, 6);
	fuc7=getbit(inst, 25, 31);

	s_imm=getbit(inst, 7, 11) | (getbit(inst, 25, 31) << 5);
	i_imm=getbit(inst, 20, 31);
	u_imm=getbit(inst, 12, 31) << 12;
	uj_imm=(getbit(inst, 12, 19)<<12) | (getbit(inst, 20, 20)<<11) | (getbit(inst, 21, 30)<<1) | (getbit(inst, 31, 31)<<20);
	sb_imm=(getbit(inst, 7, 7)<<11) | (getbit(inst, 8, 11)<<1) | (getbit(inst, 25, 30)<<5) | (getbit(inst, 31, 31)<<12);


	s_imm_ext=ext_signed_ll(s_imm, 11);
	i_imm_ext=ext_signed_ll(i_imm, 11);
	u_imm_ext=ext_signed_ll(u_imm, 31);
	uj_imm_ext=ext_signed_ll(uj_imm, 20);
	sb_imm_ext=ext_signed_ll(sb_imm, 12);

	uj_imm_ext = uj_imm_ext >> 2;
	sb_imm_ext = sb_imm_ext >> 2;

	if(OP==0x17)
	{
		u_imm_ext = u_imm_ext >> 2;
	}

	if(OP==0x67)
	{
		i_imm_ext = i_imm_ext >> 2;
	}
	op2_sel.set_input((ll)reg[rs2], 0);
	op2_sel.set_input(s_imm_ext, 1);
	op2_sel.set_input(i_imm_ext, 2);
	op2_sel.set_input(u_imm_ext, 3);
	op2_sel.set_input(sb_imm_ext, 5);
	op2_sel.set_input(uj_imm_ext, 4);
	op1_sel.set_input(D_REG.PC, 1);
	op1_sel.set_input((ll)reg[rs1], 0);
	//printf("op1 input0: %llx\n", op1_sel.input[0]);

	if(debug_flag)
	{
		printf("-------------------  DECODE  ---------------------\n");
		printf("Addr: %08llx  instruction: %08x    \n", D_REG.PC<<2, inst);
		printOP(inst);
	}

	/* op1_sel */
	switch(OP)
	{
		case 0x6f:
		case 0x17: op1_sel.set_signal(1);break;
		default: op1_sel.set_signal(0);break;
	}
	/* op2_sel */
	switch(OP)
	{
		// rs2 :  R
		case OP_RW:
		case 0x33: op2_sel.set_signal(0);break;
		// Stype imm : S
		case 0x23: op2_sel.set_signal(1);break;
		// Itype imm : I
		case 0x3:
		case 0x13:
		case 0x1f:
		case 0x1b:
		case 0x67: op2_sel.set_signal(2);break;
		// Utype imm : U
		case 0x17:
		case 0x37: op2_sel.set_signal(3);break;
		// UJtype imm : UJ
		case 0x6f: op2_sel.set_signal(4);break;
		// SBtype imm : SB
		case 0x63: op2_sel.set_signal(5);break;
		default:break;
	}

	/* choose ALU FUNC */

	if((OP==0x33 && fuc7==0x00 && fuc3==0x00)
		|| (OP==0x13 && fuc3==0x00)
		|| (OP==0x03)
		|| (OP==0x67)
		|| (OP==0x23)
		|| (OP==0x17)
		|| (OP==0x6f))
	{
		ALU_OP=ALU_ADD;
	}

	if(OP==0x33 && fuc3==0x00 && fuc7==0x01)
	{
		ALU_OP=ALU_MUL;
	}

	if(OP==0x33 && fuc3==0x00 && fuc7==0x20)
	{
		ALU_OP=ALU_SUB;
	}

	if((OP==0x33 && fuc3==0x01 && fuc7==0x00)
		|| (OP==0x13 && fuc3==0x01 && fuc7==0x00))
	{
		ALU_OP=ALU_SHIFTL;
	}

	if(OP==0x33 && fuc3==0x01 && fuc7==0x01)
	{
		ALU_OP=ALU_MULH;
	}

	if((OP==0x33 && fuc3==0x02 && fuc7==0x00)
		|| (OP==0x13 && fuc3==0x02))
	{
		ALU_OP=ALU_SLT;
	}

	if((OP==0x33 && fuc3==0x04 && fuc7==0x00)
		|| (OP==0x13 && fuc3==0x04))
	{
		ALU_OP=ALU_XOR;
	}

	if(OP==0x33 && fuc3==0x04 && fuc7==0x01)
	{
		ALU_OP=ALU_DIV;
	}

	if((OP==0x33 && fuc3==0x05 && fuc7==0x00)
		|| (OP==0x13 && fuc3==0x05 &&fuc7==0x01))
	{
		ALU_OP=ALU_SHIFTR_L;
	}

	if((OP==0x33 && fuc3==0x05 && fuc7==0x20)
		|| (OP==0x13 && fuc3==0x05 &&fuc7==0x10))
	{
		ALU_OP=ALU_SHIFTR_A;
	}

	if((OP==0x33 && fuc3==0x06 && fuc7==0x00)
		|| (OP==0x13 && fuc3==0x06))
	{
		ALU_OP=ALU_OR;
	}

	if(OP==0x33 && fuc3==0x06 && fuc7==0x01)
	{
		ALU_OP=ALU_MOD;
	}

	if((OP==0x33 && fuc3==0x07)
		|| (OP==0x13 && fuc3==0x07))
	{
		ALU_OP=ALU_AND;
	}

	if(OP==0x1B)
	{
		ALU_OP=ALU_ADDIW;
	}

	if(OP==0x63&&fuc3==0x00)
	{
		ALU_OP=ALU_BEQ;
	}

	if(OP==0x63&&fuc3==0x01)
	{
		ALU_OP=ALU_BNE;
	}

	if(OP==0x63&&fuc3==0x04)
	{
		ALU_OP=ALU_BLT;
	}

	if(OP==0x63&&fuc3==0x05)
	{
		ALU_OP=ALU_BGE;
	}

	if(OP==0x37)
	{
		ALU_OP=ALU_OP2;
	}

	if(OP==0x67||OP==0x6f)
	{
		D_REG.alu_signal = 1;
		D_REG.pc_signal = 2;
	}


	if(OP==OP_RW)
	{
		if(fuc3==4)
		{
			ALU_OP = ALU_DIVW;
		}
		else if(fuc3 == 0)
		{
			if(fuc7==0)
			{
				ALU_OP = ALU_ADDW;
			}
			else if(fuc7==32)
			{
				ALU_OP = ALU_SUBW;
			}
			else if(fuc7==1)
			{
				ALU_OP = ALU_MULW;
			}
		}
	}

	if(OP==OP_IW)
	{
		if(fuc3==1)
		{
			ALU_OP = ALU_SLLIW;
		}
	}


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

	if(OP!=0x23 && OP!=0x63)
		D_REG.ctrl_W_WriteReg = 1;

	D_REG.OP1 = op1_sel.run();
	D_REG.OP2 = op2_sel.run();

	D_REG.ALU_OP=ALU_OP;
	D_REG.RS2 = (ll)reg[rs2];
	if(OP!=0x63 && OP!=0x23)
		D_REG.rd = rd;
	if(OP!=0x17 && OP!=0x37 && OP!=0x6f)
		D_REG.rs1 = rs1;
	if(OP==0x33 || OP==0x23 || OP==0x63 || OP==OP_RW)
		D_REG.rs2 = rs2;

	//printf("%d %d\n", D_REG.OP1, D_REG.OP2);

	if(D_REG.IR == 0)
	{
		D_REG.clearReg();
	}
}

//执行
void EX()
{
	if(debug_flag)
	{
		printf("-------------------  EXE  ---------------------\n");
		printf("Addr: %08llx  instruction: %08x    \n", E_REG.PC<<2, E_REG.IR);
		printOP(E_REG.IR);
	}
	//printf("%d\n", E_REG.ALU_OP);
	ll alu_out;
	int COND_J = 0;
	ll OP1 = E_REG.OP1;
	ll OP2 = E_REG.OP2;
	switch(E_REG.ALU_OP)
	{
		case ALU_ADD:
			alu_out = OP1 + OP2;
			break;
		case ALU_SUB:
			alu_out = OP1 - OP2;
			break;
		case ALU_MUL:
			alu_out = OP1 * OP2;
			break;
		case ALU_MULH:
		{
			unsigned ll cl = 0;
			unsigned ll ch = 0;
			unsigned ll m1 = (unsigned ll)(OP1);
			unsigned ll m2 = (unsigned ll)(OP2);
			int ii = 0;
			for(ii=0;ii<64;ii++)
			{
				unsigned ll tmp = m1 & ((m2>>ii)&1);
				unsigned ll low = tmp<<ii;
				unsigned ll high = tmp>>(64-ii);
				if((low>>63)==1 && (cl>>63)==1)
				{
					ch = ch + 1;
				}
				cl = cl + low;
				ch = ch + high;
			}
			alu_out = (ll)ch;
		}
			break;
		case ALU_DIV:
			alu_out = OP1 / OP2;
			break;
		case ALU_MOD:
			alu_out = OP1 % OP2;
			break;
		case ALU_SHIFTL:
			alu_out = OP1 << OP2;
			break;
		case ALU_SHIFTR_L:
		{
			unsigned ll u_op1 = (unsigned ll)OP1;
			alu_out = u_op1 >> OP2;
		}
			break;
		case ALU_SHIFTR_A:
			alu_out = OP1 >> OP2;
			break;
		case ALU_AND:
			alu_out = OP1 & OP2;
			break;
		case ALU_OR:
			alu_out = OP1 | OP2;
			break;
		case ALU_XOR:
			alu_out = OP1 ^ OP2;
			break;
		case ALU_SLT:
			if(OP1 < OP2)
				alu_out = 1;
			else
				alu_out = 0;
			break;
		case ALU_ADDIW:
		{
			unsigned int tmp = (unsigned int)(OP1+OP2);
			alu_out = ext_signed_ll(tmp, 31);
		}
			break;
		case ALU_BEQ:
			if(OP1 == E_REG.RS2)
				COND_J = 1;
			else
				COND_J = 0;
			break;
		case ALU_BNE:
			if(OP1 != E_REG.RS2)
				COND_J = 1;
			else
				COND_J = 0;
			break;
		case ALU_BLT:
			if(OP1 < E_REG.RS2)
				COND_J = 1;
			else
				COND_J = 0;
			break;
		case ALU_BGE:
			if(OP1 >= E_REG.RS2)
				COND_J = 1;
			else
				COND_J = 0;
			break;
		case ALU_OP1:
			alu_out = OP1;
			break;
		case ALU_OP2:
			alu_out = OP2;
			break;
		case ALU_ADDW:
		{
			int ta = (int)OP1;
			int tb = (int)OP2;
			int tc = ta + tb;
			unsigned td = (unsigned int)tc;
			alu_out = ext_signed_ll(td, 31);
		}
			break;
		case ALU_SUBW:
		{
			int ta = (int)OP1;
			int tb = (int)OP2;
			int tc = ta - tb;
			unsigned td = (unsigned int)tc;
			alu_out = ext_signed_ll(td, 31);
		}
			break;
		case ALU_MULW:
		{
			int ta = (int)OP1;
			int tb = (int)OP2;
			int tc = ta * tb;
			unsigned td = (unsigned int)tc;
			alu_out = ext_signed_ll(td, 31);
		}
			break;
		case ALU_DIVW:
		{
			int ta = (int)OP1;
			int tb = (int)OP2;
			int tc = ta / tb;
			unsigned td = (unsigned int)tc;
			alu_out = ext_signed_ll(td, 31);
		}
			break;
		case ALU_SLLIW:
		{
			ll tc = OP1 << OP2;
			unsigned int td = (unsigned int)tc;
			alu_out = ext_signed_ll(td, 31);
		}
			break;
		default:
			break;
	}

	pc_sel.set_input(alu_out, 2);
	alu_sel.set_input(alu_out, 0);
	alu_sel.set_input(E_REG.PC+1, 1);
	alu_sel.set_signal(E_REG.alu_signal);

	if(COND_J==1)
	{
		E_REG.pc_signal = 1;
		pc_sel.set_input(E_REG.PC + OP2, 1);
		pc_sel.set_signal(1);
		E_REG.ctrl_F_Jump = 1;
	}


	E_REG.ALUOUT = alu_sel.run();
	if(E_REG.IR == 0)
	{
		E_REG.clearReg();
	}

}

//访问存储器
void MEM()
{
	if(debug_flag)
	{
		printf("-------------------  MEM  ---------------------\n");
		printf("Addr: %08llx  instruction: %08x    \n", M_REG.PC<<2, M_REG.IR);
		printOP(M_REG.IR);
	}
	ll waddr = M_REG.ALUOUT >> 2;
	ll wdata = M_REG.RS2;
	unsigned ll dst = (gp - 2016)>>2;
	unsigned int dstVal = memory[dst];

	wb_sel.set_input(M_REG.ALUOUT, 0);

	if(M_REG.ctrl_M_MemWrite>0)
	{
		unsigned ll one = 1;
		switch(M_REG.ctrl_M_MemWrite)
		{
			case 1:
				wdata = wdata & ((one << 8) - 1);
				memory[waddr] = (unsigned int)(wdata);
				break;
			case 2:
				wdata = wdata & ((one << 16) - 1);
				memory[waddr] = (unsigned int)(wdata);
				break;
			case 3:
				wdata = wdata & ((one << 32) - 1);
				memory[waddr] = (unsigned int)(wdata);
				break;
			case 4:
			{
				unsigned int dh = (unsigned int)(wdata >> 32);
				unsigned int dl = (unsigned int)wdata;
				memory[waddr] = dl;
				memory[waddr + 1] = dh;
			}
				break;
			default:
				break;
		}
		M_REG.ctrl_W_WriteReg = 0;
		M_REG.ctrl_M_MemWrite = 0;
	}
	if(M_REG.ctrl_M_MemRead>0)
	{
		ll rdata = 0;
		unsigned int rdataL = 0;
		unsigned int rdataH = 0;
		switch(M_REG.ctrl_M_MemRead)
		{
			case 1:
				rdataL = memory[waddr];
				rdataL = rdataL & ((1 << 8) - 1);
				rdata = ext_signed_ll(rdataL, 7);
				break;
			case 2:
				rdataL = memory[waddr];
				rdataL = rdataL & ((1 << 16) - 1);
				rdata = ext_signed_ll(rdataL, 15);
				break;
			case 3:
				rdataL = memory[waddr];
				rdata = ext_signed_ll(rdataL, 31);
				break;
			case 4:
			{
				rdataL = memory[waddr];
				rdataH = memory[waddr+1];
				unsigned ll rrh = (unsigned ll)rdataH;
				unsigned ll rl = (unsigned ll)rdataL;
				unsigned ll rh = (unsigned ll)(rrh << 32);
				rdata = (ll)(rl | rh);
			}
				break;
			default:
				break;
		}
		wb_sel.set_input(rdata, 1);
		wb_sel.set_signal(1);

		M_REG.ctrl_M_MemRead = 0;
	}

	M_REG.WBData = wb_sel.run();

	if(M_REG.IR == 0)
	{
		M_REG.clearReg();
	}
}


//写回
void WB()
{
	if(debug_flag)
	{
		printf("-------------------  WB  ---------------------\n");
		printf("Addr: %08llx  instruction: %08x    \n", W_REG.PC<<2, W_REG.IR);
		printOP(W_REG.IR);
	}
	if(W_REG.ctrl_W_WriteReg==1)
	{
		//printf("WBData:%lx\n", W_REG.WBData);
		reg[W_REG.rd] = (unsigned ll)(W_REG.WBData);
		W_REG.ctrl_W_WriteReg = 0;
	}
	if(W_REG.IR == 0)
	{
		W_REG.clearReg();
	}
}


void printOP(unsigned int inst)
{
	fuc3=getbit(inst,12,14);
	OP=getbit(inst, 0, 6);
	fuc7=getbit(inst, 25, 31);
	rd=getbit(inst,7,11);
	rs1=getbit(inst, 15, 19);
	rs2=getbit(inst, 20, 24);
	//printf("rs1:%d rs2:%d rd:%d\n", rs1, rs2, rd);

	if(OP==OP_R)
	{
		/* add rd, rs1, rs2 */
		/* R[rd] <- R[rs1] + R[rs2] */
		if(fuc3==F3_ADD&&fuc7==F7_ADD)
		{
			printf("add rd, rs1, rs2\n");
		}
		/* mul rd, rs1, rs2 */
		/* R[rd] <- (R[rs1] * R[rs2])[31:0] */
		else if(fuc3==F3_MUL&&fuc7==F7_MSE)
		{
			printf("mul rd, rs1, rs2\n");
		}
		/* sub rd, rs1, rs2 */
		/* R[rd] <- R[rs1] - R[rs2] */
		else if(fuc3==F3_SB)
		{
			printf("sub rd, rs1, rs2 \n");
		}
		/* sll rd, rs1, rs2 */
		/* R[rd] <- R[rs1] << R[rs2] */
		else if(fuc3==1 && fuc7==0)
		{
			printf("sll rd, rs1, rs2\n");
		}
		/* mulh rd, rs1, rs2 */
		else if(fuc3==1&&fuc7==1)
		{
			printf("mulh rd, rs1, rs2\n");
		}
		/* slt rd, rs1, rs2 */
		else if(fuc3==2&&fuc7==0)
		{
			printf("slt rd, rs1, rs2\n");
		}
		/* xor rd, rs1, rs2 */
		else if(fuc3==4 && fuc7==0)
		{
			printf("xor rd, rs1, rs2 \n");
		}
		/* div rd, rs1, rs2 */
		else if(fuc3==4&&fuc7==1)
		{
			printf("div rd, rs1, rs2\n");
		}
		/* srl rd, rs1, rs2 */
		/* logical right shift, supplement 0 */
		else if(fuc3==5&&fuc7==0)
		{
			printf("srl rd, rs1, rs2\n");
		}
		/* sra rd, rs1, rs2 */
		/* algorithm right shift, supplement sign bit */
		else if(fuc3==5&&fuc7==32)
		{
			printf("sra rd, rs1, rs2\n");
		}
		/* or rd, rs1, rs2 */
		else if(fuc3==6&&fuc7==0)
		{
			printf("or rd, rs1, rs2\n");
		}
		/* rem rd, rs1, rs2 */
		else if(fuc3==6&&fuc7==1)
		{
			printf("rem rd, rs1, rs2\n");
		}
		/* and rd, rs1, rs2 */
		else if(fuc3==7&&fuc7==0)
		{
			printf("and rd, rs1, rs2 \n");
		}
	}
	else if(OP==OP_I)
    {
    	/* addi rd, rs1, imm */
        if(fuc3==F3_SB)
        {
        	printf("addi rd, rs1, imm\n");
        }
        /* slli rd, rs1, imm */
        else if(fuc3==1)
        {
        	printf("slli rd, rs1, imm\n");
        }
        /* slti rd, rs1, imm */
        else if(fuc3==2)
        {
        	printf("slti rd, rs1, imm\n");
        }
        /* xori rd, rs1, imm */
        else if(fuc3==4)
        {
        	printf("xori rd, rs1, imm\n");
        }
        /* srli rd, rs1, imm */
        /* supplement 0 */
        else if(fuc3==5&&fuc7==0)
        {
        	printf("srli rd, rs1, imm\n");
        }
        /* srai rd, rs1, imm */
        else if(fuc3==5)
        {
        	printf("srai rd, rs1, imm\n");
        }
        /* ori rd, rs1, imm */
        else if(fuc3==6)
        {
        	printf("ori rd, rs1, imm\n");
        }
        /* andi rd, rs1, imm */
        else if(fuc3==7)
        {
        	printf("andi rd, rs1, imm\n");
        }
    }
    else if(OP==OP_SW)
    {
    	/* sb, rs2, offset(rs1) */
    	if(fuc3==0)
    	{
    		printf("sb, rs2, offset(rs1)\n");
    	}
    	/* sh rs2, offset(rs1) */
    	else if(fuc3==1)
    	{    		
    		printf("sh rs2, offset(rs1)\n");
    	}
    	/* sw rs2, offset(rs1) */
    	else if(fuc3==2)
    	{
    		printf("sw rs2, offset(rs1)\n");
    	}
    	/* sd rs2, offset(rs1) */
    	else
    	{
    		printf("sd rs2, offset(rs1)\n");
    	}
    }
    else if(OP==OP_LW)
    {
    	/* lb rd, offset(rs1) */
        if(fuc3==0)
        {
        	printf("lb rd, offset(rs1)\n");
        }
        /* lh rd, offset(rs1) */
        else if(fuc3==1)
        {
        	printf("lh rd, offset(rs1)\n");
        }
        /* lw rd, offset(rs1) */
        else if(fuc3==2)
        {
        	printf("lw rd, offset(rs1)\n");
        }
        /* ld rd, offset(rs1) */
        else
        {
        	printf("ld rd, offset(rs1)\n");
        }
    }
    else if(OP==OP_BEQ)
    {
    	/* beq rs1, rs2, offset */
        if(fuc3==F3_BEQ)
        {
        	printf("beq rs1, rs2, offset\n");	
        }
        /* bne rs1, rs2, offset */
        else if(fuc3==1)
        {
        	printf("bne rs1, rs2, offset\n");
        }
        /* blt rs1, rs2, offset */
        else if(fuc3==4)
        {
        	printf("blt rs1, rs2, offset\n");
        }
        /* bge rs1, rs2, offset */
        else
        {
        	printf("bge rs1, rs2, offset\n");
        }
    }
    else if(OP==OP_JAL)
    {
    	/* jal rd, imm */
    	printf("jal rd, imm  \n");
    }
    else if(OP==OP_LUI)
    {
		/* lui rd, offset */
		printf("lui rd, offset \n");
    }
    else if(OP==OP_AUIPC)
    {
    	/* auipc rd, offset */
    	printf("auipc rd, offset\n");
    }
    else if(OP==OP_IW)
    {
    	if(fuc3==0)
    	{
    		/* addiw rd, rs1, imm */
	    	printf("addiw rd, rs1, imm\n");
    	}
    	else if(fuc3==1)
    	{
    		/* slliw rd rs1, imm */
    		printf("slliw rd, rs1, imm\n");
    	}
    	else if(fuc3==5)
    	{
    		/* srliw rd, rs1, imm */
    		printf("srliw rd, rs1, imm\n");
    	}
    	else if(fuc3==5 && fuc7==32)
    	{
    		printf("sraiw rd, rs1, imm\n");
    	}
    }
    else if(OP==OP_JALR)
    {
    	/* jalr rd, rs1, imm */
    	printf("jalr rd, rs1, imm\n"); 
    }
    else if(OP==OP_RW)
    {
    	if(fuc3==4)
    	{
    		printf("divw rd, rs1, rs2\n");
    	}
    	else
    	{
	    	if(fuc7==0)
	    	{
		    	printf("addw rd, rs1, rs2\n");
	    	}
	    	else if(fuc7==32)
	    	{
	    		printf("subw rd, rs1, rs2\n");
	    	}
	    	else if(fuc7==1)
	    	{
	    		printf("mulw rd, rs1, rs2\n");
	    	}
    	}
    }
}