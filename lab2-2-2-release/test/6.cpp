#include<stdio.h>
int result=15;
int temp=0;
int main()
{
	int i=0;
	for(i=3;i<=15;i+=3)
	{
		temp=result/i;
	}
	return 0;
}


/*

li      a5,3
sw      a5,-20(s0)
lw      a4,-20(s0)
li      a5,15
blt     a5,a4,10194 <main+0x44>
lw      a4,-2040(gp) # 11508 <result>
lw      a5,-20(s0)
divw    a4,a4,a5
sw      a4,-2016(gp) # 11520 <_fbss>
lw      a5,-20(s0)
addiw   a5,a5,3
sw      a5,-20(s0)
j       10168 <main+0x18>

*/