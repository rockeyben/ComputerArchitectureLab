#include "stdio.h"
#include "stdlib.h"
#include "time.h"

void Matmul(int c[][1000], int a[][1000], int b[][1000]){
	int i, j, k;
	int len = 1000;
	for(i = 0; i < len; i++)
		for(j = 0; j < len; j++)
			for(k = 0; k <len; k++){
				c[i][j] += a[i][k] * b[k][j];
				//printf("%d\n", c[i][j]);
			}
}


int main()
{
	FILE * p = fopen("numPool.txt", "r");
	if(p == NULL){
		printf("no such file\n");
		return -1;
	}
	int Mat1[1000][1000];
	int r;
	for(r = 0; r < 1000; r++){
		fread(Mat1[r], sizeof(int), 1000, p);
	}

	int Mat2[1000][1000];
	fseek(p, 1000 * 4, SEEK_SET);
	for(r = 0; r < 1000; r++){
		fread(Mat2[r], sizeof(int), 1000, p);
	}

	fclose(p);

	/*
	int i, j;
	for(i = 0; i < 1000; i++)
		for(j = 0; j < 1000; j++)
			printf("%d\n", Mat2[i][j]);*/

	printf("input loop nums\n");
	int loopnum;
	scanf("%d", &loopnum);
	

	double totaltime = 0.0;
	

	int (*c)[1000]=(int(*)[1000])malloc(sizeof(int)*1000*1000);

	int _;

	for(_ = 0; _ < loopnum; _++){
		double startsec = time(0);
		Matmul(c, Mat1, Mat2);
		double endsec = time(0);
		totaltime += endsec - startsec;
	}


	if (totaltime <= 0) {
		printf("Insufficient duration- Increase the LOOP count\n");
		return -1;
	}

	printf("%lf\n", totaltime);
	//KIPS = (100.0*loopnum)/totaltime;
	return 0;
}