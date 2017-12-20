#include <intrin.h>
#include <iostream>
using namespace std;

int main()
{
	__m128i arr[1] = { 0 };
	short * s_arr = (short*)arr;
	for (int i = 0; i < 8; i++)
	{
		s_arr[i] = 12;
	}

	int res = _mm_cvtsi128_si32(arr[0]);

	return 0;
}