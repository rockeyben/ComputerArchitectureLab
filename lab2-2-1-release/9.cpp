#include<stdio.h>
int a=1,b=2,c=3;

void test()
{
	a=b+c;
	b=a*c;
	c=b-c;
	a=b/c;
}

int main()
{
	test();
	return 0;
}