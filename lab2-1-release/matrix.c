#include <stdio.h>

int a[3][3];
int b[3][3];
int c[3][3];

int main()
{
	int i, j, k;
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
		{
			a[i][j] = i + j;
			b[i][j] = i - j;
			c[i][j] = 0;
		}
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			for(k = 0; k < 3; k++)
				c[i][j] += a[i][k] * b[k][j];
	return 0;
}