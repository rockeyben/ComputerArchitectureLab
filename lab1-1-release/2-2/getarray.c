#include "stdio.h"
#include "stdlib.h"
int main()
{
	FILE * p = fopen("numPool.txt", "w");
	int i;
	for(i = 0; i < 10000000; i++){
		int a = random()%100000;
		fwrite(&a, sizeof(int), 1, p);
	}
	//fwrite(arr, sizeof(int), 100000, p);
	fclose(p);
	return 0;
}