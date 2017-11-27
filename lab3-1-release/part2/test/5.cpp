#include<stdio.h>
int result[6]={0,1,2,3,4,5};
int main()
{
	int i=0;
	for(i=1;i<=5;i+=1)
	{
		result[i]=result[i]/i;
	}
	return 0;
}

/*

li      a5,3
sw      a5,-20(s0)
lw      a4,-20(s0)
li      a5,15
blt     a5,a4,101ac <main+0x6c>
lui     a5,0x11
lw      a4,-20(s0)
slli    a4,a4,0x2
addi    a5,a5,-560 # 10dd0 <_fdata>
add     a5,a4,a5
lw      a4,0(a5)
lw      a5,-20(s0)
divw    a4,a4,a5
lui     a5,0x11
lw      a3,-20(s0)
slli    a3,a3,0x2
addi    a5,a5,-560 # 10dd0 <_fdata>
add     a5,a3,a5
sw      a4,0(a5)
lw      a5,-20(s0)
addiw   a5,a5,3
sw      a5,-20(s0)
j       10158 <main+0x18>

*/