#include "stdio.h"
#include "string.h"
#include "time.h"



int partition(int * arr, int s, int e)
{
	int i = s - 1;
	int j = s;
	int tmp;
	for(j; j < e; j++)
	{
		if(arr[j] <= arr[e]){
			i = i + 1;
			tmp = arr[i];
			arr[i] = arr[j];
			arr[j] = tmp;
		}
	}
	tmp = arr[i+1];
	arr[i+1] = arr[e];
	arr[e] = tmp;
	return i;
}


void q_sort(int * arr, int s, int e)
{
	if(s < e)
	{
		int divide = partition(arr, s, e);
		q_sort(arr, s, divide);
		q_sort(arr, divide+1, e);
	}
	else
		return ;
}


int main()
{
	FILE * p = fopen("array.txt", "r");
	if(p == NULL){
		printf("no such file\n");
		return -1;
	}
	int array[100000];
	fread(array, sizeof(int), 100000, p);
	fclose(p);

	printf("input loop nums\n");
	int loopnum;
	scanf("%d", &loopnum);
	

	double totaltime = 0.0;
	

	int _;
	//for(_ = 0; _ < 100000; _++)
	//	printf("%d\n", array[_]);
	for(_ = 0; _ < loopnum; _++){
		int array_sort[100000];
		memcpy(array_sort, array, 100000 * 4);
		//printf("haha\n");
		double startsec = time(0);
		q_sort(array_sort, 0, 100000 - 1);
		double endsec = time(0);
		totaltime += endsec - startsec;
		int aa;
		//for(aa = 0; aa < 100000; aa++)
		//printf("%d\n", array_sort[aa]);
	}

	

	if (totaltime <= 0) {
		printf("Insufficient duration- Increase the LOOP count\n");
		return -1;
	}

	printf("qsort in c cost %lf\n s", totaltime);
	//KIPS = (100.0*loopnum)/totaltime;
	return 0;
}
