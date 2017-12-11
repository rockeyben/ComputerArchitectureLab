#include<stdio.h>
int result[6]={1,2,3,4,5,6};
int temp=2;
int main()
{
	result[1]=result[1]+1;
	temp=temp*1;
	result[2]=result[2]+1;
	temp=temp*2;
	result[3]=result[3]+1;
	temp=temp*3;
	result[4]=result[4]+1;
	temp=temp*4;
	result[5]=result[5]+1;
	temp=temp*5;
	return 0;
}