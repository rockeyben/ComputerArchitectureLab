#include "Simulation.h"
#include "stdio.h"
#include "Read_Elf.h"

#define ll long long
using namespace std;

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

//extern void read_elf();
extern FILE *file;
//指令运行数
long long inst_num=0;

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
		//printf("%d before char form: %x\n",i, uic);
		int sh = i * 8;
		ui = ui | (uic << sh);
		//printf("after ui : %x\n", ui);
	}
	return ui;
}

void load_memory()
{
	/*
	fseek(file,cadr,SEEK_SET);
	fread(&memory[vadr>>2],1,csize,file);

	vadr=vadr>>2;
	csize=csize>>2;
	fclose(file);
	*/
	for(int i = 0; i < (code_size >> 2); i++)
	{
		memory[i+(code_segment>>2)] = fetch_ui(all_start+code_offset+i*4);
	}
	for(int i = 0; i < (data_size >> 2); i++)
	{
		memory[i+(data_segment>>2)] = fetch_ui(all_start+data_offset+i*4);
	}
	/*
	int tt = 0;
	for(tt = 0; tt < (code_size>>2); tt++)
	{
		printf("%x ", memory[tt+(code_segment>>2)]);
	}
	printf("\n-----------GT\n");
	for(tt = 0; tt < code_size; tt++)
	{
		printf("%x ", all_start[tt+code_offset]);
	}
	*/
}

int main(int argc, char*argv[])
{	
	//解析elf文件
	read_elf(argv[1]);
	
	//加载内存
	load_memory();

	//设置入口地址
	PC=main_in>>2;	
	
	//设置全局数据段地址寄存器
	reg[3]=gp;
	
	reg[2]=MAX/2;//栈基址 （sp寄存器）

	simulate();
	debug();
	cout <<"simulate over!"<<endl;

	return 0;
}

void debug()
{
	for(int i = 0; i < 32; i++)
		printf("%d. %llx  ", i, reg[i]);
	printf("\n");
	unsigned int g =reg[3]>>2;
	for(unsigned int i = 0; i < 15; i++)
	{
		printf("Mem%lx.%d   ", (g+i) << 2, memory[i]);
	}
	printf("\n");
}

void simulate()
{
	//结束PC的设置
	long long end=(long long)main_end/4;
	while(PC!=end)
	{
		//运行
		IF();
		ID();
		EX();
		MEM();
		WB();

		//更新中间寄存器
		IF_ID=IF_ID_old;
		ID_EX=ID_EX_old;
		EX_MEM=EX_MEM_old;
		MEM_WB=MEM_WB_old;

        if(exit_flag==1)
            break;

        reg[0]=0;//一直为零
        debug();
	}
}


//取指令
void IF()
{
	//write IF_ID_old
	IF_ID.inst=memory[PC];
	IF_ID_old.PC=PC;
	PC=PC+1;
}

//译码
void ID()
{
	//Read IF_ID
	unsigned int inst=IF_ID.inst;
	int EXTop=0;
	unsigned int EXTsrc=0;

	char RegDst,ALUop,ALUSrc;
	char Branch,MemRead,MemWrite;
	char RegWrite,MemtoReg;

	rd=getbit(inst,7,11);
	fuc3=getbit(inst,12,14);
	OP = getbit(inst, 0, 6);
	fuc7=getbit(inst, 25, 31);
	//....

	unsigned int rs1;
	unsigned int rs2;

	//printf("%05llx  %08x  %x   %x  %x  \n",(IF_ID_old.PC)<<2, inst, OP, fuc3, fuc7);
	printf("%05llx  %08x    ", (IF_ID_old.PC)<<2, inst);
	rs1 = getRs1(inst);
	rs2 = getRs2(inst);
	if(OP==OP_R)
	{
		ll a = (ll)reg[rs1];
		ll b = (ll)reg[rs2];
		/* add rd, rs1, rs2 */
		/* R[rd] <- R[rs1] + R[rs2] */
		if(fuc3==F3_ADD&&fuc7==F7_ADD)
		{
			printf("add rd, rs1, rs2\n");
			/*
            EXTop=0;
			RegDst=0;
			ALUop=ALU_ADD;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;
			*/
			ll c = a + b;
			reg[rd] = (unsigned ll)c;
		}
		/* mul rd, rs1, rs2 */
		/* R[rd] <- (R[rs1] * R[rs2])[31:0] */
		else if(fuc3==F3_MUL&&fuc7==F7_MSE)
		{
			printf("mul rd, rs1, rs2\n");
			/*
            EXTop=0;
			RegDst=0;
			ALUop=ALU_MULL;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;
			*/
			ll a = (ll)reg[rs1];
			ll b = (ll)reg[rs2];
			ll c = a * b;
			reg[rd] = (unsigned ll)c;
		}
		/* sub rd, rs1, rs2 */
		/* R[rd] <- R[rs1] - R[rs2] */
		else if(fuc3==F3_SB)
		{
			printf("sub rd, rs1, rs2 \n");
			/*
			EXTop=0;
			RegDst=0;
			ALUop=ALU_SUB;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;
			*/
			ll c = a - b;
			reg[rd] = (unsigned ll)c;
		}
		/* sll rd, rs1, rs2 */
		/* R[rd] <- R[rs1] << R[rs2] */
		else if(fuc3==1 && fuc7==0)
		{
			printf("sll rd, rs1, rs2\n");
			/*
			EXTop=0;
			RegDst=0;
			ALUop=ALU_SHIFTL;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;
			*/
			if(b < 0 || b > 64)
			{
				printf("sll, invalid shamt!\n");
				b=0;
				//break;
			}
			ll c = a << b;
			reg[rd] = (unsigned ll)c;
		}
		/* mulh rd, rs1, rs2 */
		else if(fuc3==1&&fuc7==1)
		{
			printf("mulh rd, rs1, rs2\n");
			/*
			EXTop=0;
			RegDst=0;
			ALUop=ALU_MULH;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;
			*/
			unsigned ll cl = 0;
			unsigned ll ch = 0;
			unsigned ll m1 = reg[rs1];
			unsigned ll m2 = reg[rs2];
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
			reg[rd] = ch;
		}
		/* slt rd, rs1, rs2 */
		else if(fuc3==2&&fuc7==0)
		{
			printf("slt rd, rs1, rs2\n");
			EXTop=0;
			RegDst=0;
			ALUop=ALU_SUB;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;

			ll c = a - b;
			if(c < 0)
			{
				reg[rd] = 1;
			}
			else
			{
				reg[rd] = 0;
			}
		}
		/* xor rd, rs1, rs2 */
		else if(fuc3==4 && fuc7==0)
		{
			printf("xor rd, rs1, rs2 \n");
			EXTop=0;
			RegDst=0;
			ALUop=ALU_XOR;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;

			ll c = a ^ b;
			reg[rd] = (unsigned ll)c;
		}
		/* div rd, rs1, rs2 */
		else if(fuc3==4&&fuc7==1)
		{
			printf("div rd, rs1, rs2\n");
			EXTop=0;
			RegDst=0;
			ALUop=ALU_DIV;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;

			ll c = a / b;
			reg[rd] = (unsigned ll)c;
		}
		/* srl rd, rs1, rs2 */
		/* logical right shift, supplement 0 */
		else if(fuc3==5&&fuc7==0)
		{
			printf("srl rd, rs1, rs2\n");
			EXTop=0;
			RegDst=0;
			ALUop=ALU_SHIFTR;
			ALUSrc=0;
			Branch=0;
			MemRead=0;
			MemWrite=0;
			RegWrite=0;
			MemtoReg=0;

			if(b < 0 || b > 64)
			{
				printf("srl, invalid shamt!\n");
				b=0;
				//break;
			}
			reg[rd] = reg[rd] >> b;
		}
		/* sra rd, rs1, rs2 */
		/* algorithm right shift, supplement sign bit */
		else if(fuc3==5&&fuc7==32)
		{
			printf("sra rd, rs1, rs2\n");
			if(b < 0 || b > 64)
			{
				printf("sra, invalid shamt!\n");
				b=0;
				//break;
			}
			ll c = a >> b;
			reg[rd] = (unsigned ll)c;
		}
		/* or rd, rs1, rs2 */
		else if(fuc3==6&&fuc7==0)
		{
			printf("or rd, rs1, rs2\n");
			reg[rd] = (unsigned ll)(a | b);
		}
		/* rem rd, rs1, rs2 */
		else if(fuc3==6&&fuc7==1)
		{
			printf("rem rd, rs1, rs2\n");
			reg[rd] = (unsigned ll)(a % b);
		}
		/* and rd, rs1, rs2 */
		else if(fuc3==7&&fuc7==0)
		{
			printf("and rd, rs1, rs2 \n");
			reg[rd] = (unsigned ll)(a & b);
		}
	}
	else if(OP==OP_I)
    {
    	ll a = (ll)reg[rs1];
       	ll imm = ext_signed_ll(getbit(inst, 20, 31), 11);
    	/* addi rd, rs1, imm */
        if(fuc3==F3_SB)
        {
        	printf("addi rd, rs1, imm\n");
        	ll c = a + imm;
        	reg[rd] = (unsigned ll)c;
        }
        /* slli rd, rs1, imm */
        else if(fuc3==1)
        {
        	printf("slli rd, rs1, imm\n");
        	if(imm < 0 || imm > 64)
        	{
        		printf("slli, invalid shamt!\n");
        		imm=0;
        		//break;
        	}
        	reg[rd] = reg[rs1] << imm;
        }
        /* slti rd, rs1, imm */
        else if(fuc3==2)
        {
        	printf("slti rd, rs1, imm\n");
        	if(a < imm)
        	{
        		reg[rd] = 1;
        	}
        	else
        	{
        		reg[rd] = 0;
        	}
        }
        /* xori rd, rs1, imm */
        else if(fuc3==4)
        {
        	printf("xori rd, rs1, imm\n");
        	reg[rd] = reg[rs1] ^ imm;
        }
        /* srli rd, rs1, imm */
        /* supplement 0 */
        else if(fuc3==5&&fuc7==0)
        {
        	printf("srli rd, rs1, imm\n");
        	if(imm > 64 || imm < 0)
        	{
        		printf("srli, invalid shamt!\n");
        		imm=0;
        		//break;
        	}
        	reg[rd] = reg[rs1] >> imm;
        }
        /* srai rd, rs1, imm */
        else if(fuc3==5)
        {
        	printf("srai rd, rs1, imm\n");
        	if(imm > 64 || imm < 0)
        	{
        		printf("srai, invalid shamt!\n");
        		imm=0;
        		//break;
        	}
        	ll c = a >> imm;
        	reg[rd] = (unsigned ll)c;
        }
        /* ori rd, rs1, imm */
        else if(fuc3==6)
        {
        	printf("ori rd, rs1, imm\n");
        	reg[rd] = reg[rs1] | (unsigned ll)imm;
        }
        /* andi rd, rs1, imm */
        else if(fuc3==7)
        {
        	printf("andi rd, rs1, imm\n");
        	reg[rd] = reg[rs1] & (unsigned ll)imm;
        }
    }
    else if(OP==OP_SW)
    {
    	ll a = (ll)reg[rs1];
    	unsigned int imm1 = getbit(inst, 7, 11);
    	unsigned int imm2 = getbit(inst, 25, 31);
    	imm1 = imm1 | (imm2 << 5);
    	ll imm = ext_signed_ll(imm1, 11);
    	ll target = (a + imm)>>2;
    	/* sb, rs2, offset(rs1) */
    	if(fuc3==0)
    	{
    		printf("sb, rs2, offset(rs1)\n");
    		unsigned int b = (unsigned int)(reg[rs2] & ( 1<<8 -1));
    		unsigned int om = memory[target];
    		om = om & (~((1<<8)-1));
    		om = om | b;  
    		memory[target] = om;
    	}
    	/* sh rs2, offset(rs1) */
    	else if(fuc3==1)
    	{    		
    		printf("sh rs2, offset(rs1)\n");
    		unsigned int b = (unsigned int)(reg[rs2] & ( 1<<16 -1));
    		unsigned int om = memory[target];
    		om = om & (~((1<<16)-1));
    		om = om | b;  
    		memory[target] = om;
    	}
    	/* sw rs2, offset(rs1) */
    	else if(fuc3==2)
    	{
    		printf("sw rs2, offset(rs1)\n");
    		unsigned int b = (unsigned int)reg[rs2]; 
    		memory[target] = b;
    	}
    	/* sd rs2, offset(rs1) */
    	else
    	{
    		printf("sd rs2, offset(rs1)\n");
    		unsigned int bl = (unsigned int)reg[rs2];
    		unsigned int bh = (unsigned int)(reg[rs2] >> 32);
    		memory[target] = bl;
    		memory[target+1] = bh;
    	}
    }
    else if(OP==OP_LW)
    {

    	ll a = (ll)(reg[rs1]);
    	ll imm = ext_signed_ll(getbit(inst, 20, 31), 11);
    	ll target = (a+imm)>>2;

    	/* lb rd, offset(rs1) */
        if(fuc3==0)
        {
        	printf("lb rd, offset(rs1)\n");
            unsigned int b = memory[target];
            b = b & ((1 << 8)-1);
            reg[rd] = ext_signed_ll(b, 7);
        }
        /* lh rd, offset(rs1) */
        else if(fuc3==1)
        {
        	printf("lh rd, offset(rs1)\n");
           	unsigned int b = memory[target];
           	b = b & ((1 << 16)-1);
           	reg[rd] = ext_signed_ll(b, 15);
        }
        /* lw rd, offset(rs1) */
        else if(fuc3==2)
        {
        	printf("lw rd, offset(rs1)   %d\n", rs1);
        	unsigned int b = memory[target];
        	//printf("offset==%d\n", imm);
        	reg[rd] = ext_signed_ll(b, 31);
        }
        /* ld rd, offset(rs1) */
        else
        {
        	printf("ld rd, offset(rs1)\n");
        	unsigned int b1 = memory[target];
        	unsigned int b2 = memory[target+1];
        	unsigned ll b11 = (unsigned ll)b1;
        	unsigned ll b22 = (unsigned ll)b2;
        	reg[rd] = (b22 << 32) | b11; 
        }
    }
    else if(OP==OP_BEQ)
    {
    	ll a = (ll)reg[rs1];
    	ll b = (ll)reg[rs2];
    	unsigned int imm1 = getbit(inst, 7, 7);
    	unsigned int imm2 = getbit(inst, 8, 11);
    	unsigned int imm3 = getbit(inst, 25, 30);
    	unsigned int imm4 = getbit(inst, 31, 31);
    	unsigned int immi = (imm2 << 1) | (imm1 << 11) | (imm3 << 5) | (imm4 << 12);
    	ll imm = ext_signed_ll(immi, 12);
    	imm = imm >> 2;
    	/* beq rs1, rs2, offset */
        if(fuc3==F3_BEQ)
        {
        	printf("beq rs1, rs2, offset\n");
			if(a==b)
			{
				PC = PC + imm - 1;
			}		
        }
        /* bne rs1, rs2, offset */
        else if(fuc3==1)
        {
        	printf("bne rs1, rs2, offset\n");
        	if(a!=b)
        	{
        		PC = PC + imm - 1;
        	}
        }
        /* blt rs1, rs2, offset */
        else if(fuc3==4)
        {
        	printf("blt rs1, rs2, offset\n");
        	if(a<b)
        	{
        		PC = PC + imm - 1;
        	}
        }
        /* bge rs1, rs2, offset */
        else
        {
        	printf("bge rs1, rs2, offset %x\n", (PC+imm-1)<<2);
        	//printf("%lld %lld %d %d\n", reg[rs1], reg[rs2], rs1, rs2);
        	if(a>=b)
        	{
        		PC = PC + imm - 1;
        	}
        }
    }
    else if(OP==OP_JAL)
    {
    	/* jal rd, imm */
    	printf("jal rd, imm  ");
		unsigned int imm1 = getbit(inst, 12, 19);
		unsigned int imm2 = getbit(inst, 20, 20);
		unsigned int imm3 = getbit(inst, 21, 30);
		unsigned int imm4 = getbit(inst, 31, 31);
		unsigned int immi = (imm3 << 1) | (imm2 << 11) | (imm1 << 12) | (imm4 << 20);
		ll imm = ext_signed_ll(immi, 20);
		imm = imm >> 2;
		printf("%x  ",  imm);
		printf("%x\n", (PC+imm-1)<<2);
		reg[rd] = PC;
		PC = PC + imm - 1;
    }
    else if(OP==OP_LUI)
    {
		/* lui rd, offset */
		printf("lui rd, offset \n");
		unsigned int imm = getbit(inst, 12, 31);
		imm = imm << 12;
		ll c = ext_signed_ll(imm, 31);
		reg[rd] = (unsigned ll)c;	
    }
    else if(OP==OP_AUIPC)
    {
    	/* auipc rd, offset */
    	printf("auipc rd, offset\n");
		unsigned int imm = getbit(inst, 12, 31);
		imm = imm << 12;
		ll c = ext_signed_ll(imm, 31);
		c = PC + c - 1;
		reg[rd] = (unsigned ll)c;
    }
    else if(OP==OP_IW)
    {
    	/* addiw rd, rs1, imm */
    	printf("addiw rd, rs1, imm\n");
    	unsigned int imm = getbit(inst, 20, 31);
    	ll c = ext_signed_ll(imm, 11);
    	ll a = (ll)reg[rs1];
    	c = a + c;
    	unsigned int rr = (unsigned int)c;
    	reg[rd] = ext_signed_ll(rr, 31);
    }
    else if(OP==OP_JALR)
    {
    	/* jalr rd, rs1, imm */
    	printf("jalr rd, rs1, imm\n");
    	unsigned int imm = getbit(inst, 20, 31);
    	ll c = ext_signed_ll(imm, 11);
    	ll a = (ll)reg[rs1];
    	c = a + c;
    	c = (c>>1) << 1;
    	reg[rd] = PC;
    	PC = (unsigned ll)c;	 
    }
    else if(OP==OP_RW)
    {
    	printf("addw rd, rs1, rs2\n");
    	int a = (int)reg[rs1];
    	int b = (int)reg[rs2];
    	int c = a + b;
    	unsigned int d = (unsigned int)c;
    	ll e = ext_signed_ll(d, 31);
    	reg[rd] = (unsigned ll)e;
    }

	//write ID_EX_old
	ID_EX_old.Rd=rd;
	ID_EX_old.Rt=rt;
	ID_EX_old.Imm=ext_signed(EXTsrc,EXTop);
	//...

	ID_EX_old.Ctrl_EX_ALUOp=ALUop;
	//....

}

//执行
void EX()
{
	//read ID_EX
	int temp_PC=ID_EX.PC;
	char RegDst=ID_EX.Ctrl_EX_RegDst;
	char ALUOp=ID_EX.Ctrl_EX_ALUOp;

	//Branch PC calulate
	//...

	//choose ALU input number
	//...

	//alu calculate
	int Zero;
	REG ALUout;
	switch(ALUOp){
	default:;
	}

	//choose reg dst address
	int Reg_Dst;
	if(RegDst)
	{

	}
	else
	{

	}

	//write EX_MEM_old
	EX_MEM_old.ALU_out=ALUout;
	EX_MEM_old.PC=temp_PC;
    //.....
}

//访问存储器
void MEM()
{
	//read EX_MEM

	//complete Branch instruction PC change

	//read / write memory

	//write MEM_WB_old
}


//写回
void WB()
{
	//read MEM_WB

	//write reg
}