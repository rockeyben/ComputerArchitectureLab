#include<stdio.h>
int a=0,b=1,c=2;
int main()
{
	a=a+b;
	a=a+2;
	
	b=b+a;
	b=b+3;
	
	c=c+a;
	c=c+4;
	return 0;
}

/*

lw      a4,-2016(gp) # 11540 <_fbss>
lw      a5,-2040(gp) # 11528 <b>
addw    a4,a4,a5
sw      a4,-2016(gp) # 11540 <_fbss>

lw      a5,-2016(gp) # 11540 <_fbss>
addiw   a4,a5,2
sw      a4,-2016(gp) # 11540 <_fbss>

lw      a4,-2040(gp) # 11528 <b>
lw      a5,-2016(gp) # 11540 <_fbss>
addw    a4,a4,a5
sw      a4,-2040(gp) # 11528 <b>

lw      a5,-2040(gp) # 11528 <b>
addiw   a4,a5,3
sw      a4,-2040(gp) # 11528 <b>

lw      a4,-2036(gp) # 1152c <c>
lw      a5,-2016(gp) # 11540 <_fbss>
addw    a4,a4,a5
sw      a4,-2036(gp) # 1152c <c>

lw      a5,-2036(gp) # 1152c <c>
addiw   a4,a5,4
sw      a4,-2036(gp) # 1152c <c>


*/