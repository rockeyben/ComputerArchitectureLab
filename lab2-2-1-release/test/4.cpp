#include<stdio.h>
int result[5]={1,2,3,4,5};
int sum=0;
int main()
{
	int i=0;
	for(i=0;i<5;i++)
	{
		result[i]=result[i]*i;
		sum+=result[i];
	}
	return 0;
}

/*

lw      a4,-20(s0)
li      a5,4
blt     a5,a4,101cc <main+0x8c>
lui     a5,0x11
lw      a4,-20(s0)
slli    a4,a4,0x2
addi    a5,a5,-528 # 10df0 <_fdata>
add     a5,a4,a5
lw      a4,0(a5)
lw      a5,-20(s0)
mulw    a4,a4,a5
lui     a5,0x11
lw      a3,-20(s0)
slli    a3,a3,0x2
addi    a5,a5,-528 # 10df0 <_fdata>
add     a5,a3,a5
sw      a4,0(a5)
lui     a5,0x11
lw      a4,-20(s0)
slli    a4,a4,0x2
addi    a5,a5,-528 # 10df0 <_fdata>
add     a5,a4,a5
lw      a4,0(a5)
lw      a5,-2024(gp) # 11568 <_fbss>
addw    a4,a4,a5
sw      a4,-2024(gp) # 11568 <_fbss>
lw      a5,-20(s0)
addiw   a5,a5,1
sw      a5,-20(s0)
j       10154 <main+0x14>

*/