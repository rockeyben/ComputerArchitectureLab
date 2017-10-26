#include <stdio.h>
int a[100];
int main(void) 
{ 
  int b = 1;
  int c = 2;
  a[1] = 1;
  a[0] = 1;
  int d = 0;
  for(d=2; d<11; d++)
  {
  	a[d] = a[d-1] + a[d-2];
  }
  return 0; 
}
