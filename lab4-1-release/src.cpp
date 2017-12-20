#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <intrin.h>
#include <time.h>
#include <Windows.h>
#define W 1920
#define H 1080
#define img_size W*H+W*H/2
#define MAX(a,b) a>b?a:b 
#define MIN(a,b) a<b?a:b

using namespace std;

__m64 zero, f;
__m128i zero_128, f_128;
__m256i zero_256, f_256;

int process_without_simd_fade(char * yuv_img);
int precess_without_simd_picture_merge(char *yuv_img1, char*yuv_img2);
int process_with_mmx_fade(char * yuv_img);
int process_with_mmx_picture_merge(char*yuv_img1, char*yuv_img2);
int process_with_sse_picture_merge(int*yuv_img1, int*yuv_img2);
int process_with_avx_picture_merge(int*yuv_img1, int*yuv_img2);
int process_with_sse_fade(int*yuv_img);
int process_with_avx_fade(int * yuv_img);

short RGBparam[3][4] = { {0, 0, 360, 128},{88,128,184,128},{455, 128, 0, 0} };
short YUVparam[3][5] = { {66,129,25,128,16},{-38,-74, 112,128,128},{112, -94, -18, 128, 128} };

__m64 RGBparam_mmx[3][4];
__m64 YUVparam_mmx[3][5];

__m128i RGBparam_sse[3][4];
__m128i YUVparam_sse[3][5];

__m256i RGBparam_avx[3][4];
__m256i YUVparam_avx[3][5];

void init_params()
{
	zero = _m_pxor(zero, zero);
	f = _m_from_int(0xffffffff);
	f = _m_punpckldq(f, f);

	zero_128 = _mm_setzero_si128();
	f_128 = _mm_set_epi64(f, f);

	zero_256 = _mm256_setzero_si256();
	f_256 = _mm256_set_m128i(f_128, f_128);

	for(int i = 0; i < 3;i++)
		for (int j = 0; j < 4; j++)
		{
			RGBparam_mmx[i][j] = _mm_set1_pi16(RGBparam[i][j]);
			RGBparam_sse[i][j] = _mm_set1_epi16(RGBparam[i][j]);
			RGBparam_avx[i][j] = _mm256_set1_epi16(RGBparam[i][j]);

		}
	for(int i = 0; i < 3; i++)
		for (int j = 0; j < 5; j++)
		{
			YUVparam_mmx[i][j] = _mm_set1_pi16(YUVparam[i][j]);
			YUVparam_sse[i][j] = _mm_set1_epi16(YUVparam[i][j]);
			YUVparam_avx[i][j] = _mm256_set1_epi16(YUVparam[i][j]);
		}
}

__m64 computeRGB_mmx(__m64 Y, __m64 U, __m64 V, int id)
{
	__m64 C, tmp1, tmp2;

	tmp1 = _m_psubsw(U, RGBparam_mmx[id][1]);
	tmp1 = _mm_mullo_pi16(tmp1, RGBparam_mmx[id][0]);
	tmp2 = _m_psubsw(V, RGBparam_mmx[id][3]);
	tmp2 = _mm_mullo_pi16(tmp2, RGBparam_mmx[id][2]);
	tmp1 = _m_paddsw(tmp1, tmp2);
	tmp1 = _m_psrawi(tmp1, 8);
	if (id == 1)
		C = _m_psubsw(Y, tmp1);
	else
		C = _m_paddsw(Y, tmp1);

	C = _m_packuswb(C, zero);
	return C;
}

__m64 computeYUV_mmx(__m64 R, __m64 G, __m64 B, int id)
{
	__m64 C, tmp, tmp1, tmp2;
	tmp = _mm_mullo_pi16(YUVparam_mmx[id][0], R);
	tmp1 = _mm_mullo_pi16(YUVparam_mmx[id][1], G);
	tmp2 = _mm_mullo_pi16(YUVparam_mmx[id][2], B);

	C = _m_paddsw(tmp, tmp1);
	C = _m_paddsw(C, tmp2);
	C = _m_paddsw(C, YUVparam_mmx[id][3]);
	C = _m_psrawi(C, 8);
	C = _m_paddsw(C, YUVparam_mmx[id][4]);
	C = _m_packuswb(C, zero);
	return C;
}

__m64 picture_merge_mmx(__m64 A, __m64 C1, __m64 C2)
{
	__m64 newC;
	__m64 tmp, tmp1;
	__m64 tmpf = _m_punpcklbw(f, zero);
	tmp = _m_psubsw(tmpf, A);
	tmp = _mm_mullo_pi16(tmp, C2);
	tmp1 = _mm_mullo_pi16(A, C1);
	tmp = _m_paddusw(tmp, tmp1);
	newC = _m_psrlwi(tmp, 8);
	newC = _m_packuswb(newC, zero);
	newC = _m_punpcklbw(newC, zero);

	return newC;
}

__m64 fade_mmx(__m64 A, __m64 C)
{
	__m64 newC;
	newC = _mm_mullo_pi16(A, C);
	newC = _m_psrlwi(newC, 8);
	return newC;
}

__m128i computeRGB_sse(__m128i Y, __m128i U, __m128i V, int id)
{
	__m128i C, tmp1, tmp2;

	tmp1 = _mm_subs_epi16(U, RGBparam_sse[id][1]);
	tmp1 = _mm_mullo_epi16(tmp1, RGBparam_sse[id][0]);
	tmp2 = _mm_subs_epi16(V, RGBparam_sse[id][3]);
	tmp2 = _mm_mullo_epi16(tmp2, RGBparam_sse[id][2]);
	tmp1 = _mm_adds_epi16(tmp1, tmp2);
	tmp1 = _mm_srai_epi16(tmp1, 8);
	if (id == 1)
		C = _mm_subs_epi16(Y, tmp1);
	else
		C = _mm_adds_epi16(Y, tmp1);

	C = _mm_packus_epi16(C, zero_128);
	return C;
}

__m128i computeYUV_sse(__m128i R, __m128i G, __m128i B, int id)
{
	__m128i C, tmp, tmp1, tmp2;
	tmp = _mm_mullo_epi16(YUVparam_sse[id][0], R);
	tmp1 = _mm_mullo_epi16(YUVparam_sse[id][1], G);
	tmp2 = _mm_mullo_epi16(YUVparam_sse[id][2], B);

	C = _mm_adds_epi16(tmp, tmp1);
	C = _mm_adds_epi16(C, tmp2);
	C = _mm_adds_epi16(C, YUVparam_sse[id][3]);
	C = _mm_srai_epi16(C, 8);
	C = _mm_adds_epi16(C, YUVparam_sse[id][4]);
	C = _mm_packus_epi16(C, zero_128);
	return C;
}

__m128i picture_merge_sse(__m128i A, __m128i C1, __m128i C2)
{
	__m128i newC;
	__m128i tmp, tmp1;
	__m128i tmpf = _mm_unpacklo_epi8(f_128, zero_128);
	tmp = _mm_subs_epi16(tmpf, A);
	tmp = _mm_mullo_epi16(tmp, C2);
	tmp1 = _mm_mullo_epi16(A, C1);
	tmp = _mm_adds_epi16(tmp, tmp1);
	newC = _mm_srli_epi16(tmp, 8);
	newC = _mm_packus_epi16(newC, zero_128);
	newC = _mm_unpacklo_epi8(newC, zero_128);

	return newC;

}

__m128i fade_sse(__m128i A, __m128i C)
{
	__m128i newC;
	newC = _mm_mullo_epi16(A, C);
	newC = _mm_srli_epi16(newC, 8);
	return newC;
}

__m256i computeRGB_avx(__m256i Y, __m256i U, __m256i V, int id)
{
	__m256i C, tmp1, tmp2;

	tmp1 = _mm256_subs_epi16(U, RGBparam_avx[id][1]);
	tmp1 = _mm256_mullo_epi16(tmp1, RGBparam_avx[id][0]);
	tmp2 = _mm256_subs_epi16(V, RGBparam_avx[id][3]);
	tmp2 = _mm256_mullo_epi16(tmp2, RGBparam_avx[id][2]);
	tmp1 = _mm256_adds_epi16(tmp1, tmp2);
	tmp1 = _mm256_srai_epi16(tmp1, 8);
	if (id == 1)
		C = _mm256_subs_epi16(Y, tmp1);
	else
		C = _mm256_adds_epi16(Y, tmp1);

	C = _mm256_packus_epi16(C, zero_256);
	return C;

}

__m256i computeYUV_avx(__m256i R, __m256i G, __m256i B, int id)
{
	__m256i C, tmp, tmp1, tmp2;
	tmp = _mm256_mullo_epi16(YUVparam_avx[id][0], R);
	tmp1 = _mm256_mullo_epi16(YUVparam_avx[id][1], G);
	tmp2 = _mm256_mullo_epi16(YUVparam_avx[id][2], B);

	C = _mm256_adds_epi16(tmp, tmp1);
	C = _mm256_adds_epi16(C, tmp2);
	C = _mm256_adds_epi16(C, YUVparam_avx[id][3]);
	C = _mm256_srai_epi16(C, 8);
	C = _mm256_adds_epi16(C, YUVparam_avx[id][4]);
	__m256i par = YUVparam_avx[id][4];
	C = _mm256_packus_epi16(C, zero_256);
	return C;
}

__m256i picture_merge_avx(__m256i A, __m256i C1, __m256i C2)
{
	__m256i newC;
	__m256i tmp, tmp1;
	__m256i tmpf = _mm256_unpacklo_epi8(f_256, zero_256);
	tmp = _mm256_sub_epi16(tmpf, A);
	tmp = _mm256_mullo_epi16(tmp, C2);
	tmp1 = _mm256_mullo_epi16(A, C1);
	tmp = _mm256_adds_epi16(tmp, tmp1);
	newC = _mm256_srli_epi16(tmp, 8);
	newC = _mm256_packus_epi16(newC, zero_256);
	newC = _mm256_unpacklo_epi8(newC, zero_256);
	return newC;
}

__m256i fade_avx(__m256i A, __m256i C)
{
	__m256i newC;
	newC = _mm256_mullo_epi16(A, C);
	newC = _mm256_srli_epi16(newC, 8);
	return newC;
}

	
int main() {

	init_params();

	ifstream fin1("./dem1.yuv", ios::binary);
	ifstream fin2("./dem2.yuv", ios::binary);
	char * yuv_img1 = (char*)malloc(img_size * sizeof(char));
	fin1.read(yuv_img1, img_size);
	char * yuv_img2 = (char*)malloc(img_size * sizeof(char));
	fin2.read(yuv_img2, img_size);
	int * yuv_sse1 = (int*)malloc(img_size * sizeof(char));
	memcpy(yuv_sse1, yuv_img1, img_size * sizeof(char));
	int * yuv_sse2 = (int*)malloc(img_size * sizeof(char));
	memcpy(yuv_sse2, yuv_img2, img_size * sizeof(char));

	process_without_simd_fade(yuv_img1);
	precess_without_simd_picture_merge(yuv_img1, yuv_img2);
	process_with_mmx_picture_merge(yuv_img1, yuv_img2);
	process_with_sse_picture_merge(yuv_sse1, yuv_sse2);
	process_with_avx_picture_merge(yuv_sse1, yuv_sse2);
	process_with_mmx_fade(yuv_img1);
	process_with_sse_fade(yuv_sse1);
	process_with_avx_fade(yuv_sse1);
	cin.get();
	return 0;
}

int process_without_simd_fade(char * yuv_img) {
	int time = 0;
	char * new_yuv = (char*)malloc(img_size * sizeof(char));
	memset(new_yuv, 0, img_size * sizeof(char));
	string filename = "./part1/no_simd/figure.yuv";
	ofstream fff(filename.c_str(), ios::out);

	DWORD totalT = 0;
	for (int Alpha = 1; Alpha <= 255; Alpha += 3)
	{
		// the format YUV420 apply same U and V to one 2x2 pixel block(Y)
		DWORD starttime = GetTickCount();
		for (int i = 0; i < W*H; i++)
		{
			unsigned char tmpY = yuv_img[i];
			unsigned char tmpU = yuv_img[W*H + i / 4];
			unsigned char tmpV = yuv_img[W*H + W*H / 4 + i / 4];
			short Y = (short)tmpY;
			short U = (short)tmpU;
			short V = (short)tmpV;

			short R, G, B, y, u, v;
			R = Y + ((360 * (V - 128)) /256);
			G = Y - ((88 * (U - 128) + 184 * (V - 128)) /256);
			B = Y + ((455 * (U - 128)) /256);

			R = MIN(255, MAX(R, 0));
			G = MIN(255, MAX(G, 0));
			B = MIN(255, MAX(B, 0));

			R = (short)Alpha * R / 256;
			G = (short)Alpha * G / 256;
			B = (short)Alpha * B / 256;
			
			y = ((66 * R + 129 * G + 25 * B + 128) /256) + 16;
			u = ((-38 * R - 74 * G + 112 * B + 128) /256) + 128;
			v = ((112 * R - 94 * G - 18 * B + 128) /256) + 128;

			y = MIN(255, MAX(y, 0));
			u = MIN(255, MAX(u, 0));
			v = MIN(255, MAX(v, 0));

			new_yuv[i] = (char)y;
			new_yuv[W*H + i / 4] = (char)u;
			new_yuv[W*H + W*H / 4 + i / 4] = (char)v;

		}
		totalT += GetTickCount() - starttime;
		fff.seekp(0, ios::end);
		fff.write(new_yuv, img_size*sizeof(char));
	}
	cout << "Total time no simd for fade in fade out: " << totalT << "ms" << endl;
	return time;
}

int precess_without_simd_picture_merge(char *yuv_img1, char*yuv_img2)
{
	int time = 0;
	char * new_yuv = (char*)malloc(img_size * sizeof(char));
	memset(new_yuv, 0, img_size * sizeof(char));

	string filename = "./part2/no_simd/figure.yuv";
	ofstream fff(filename.c_str(), ios::out);

	DWORD totalT = 0;
	for (int Alpha = 1; Alpha <= 255; Alpha += 3)
	{
		// the format YUV420 apply same U and V to one 2x2 pixel block(Y)
		DWORD starttime = GetTickCount();
		for (int i = 0; i < W*H; i++)
		{
			unsigned char tmpY1 = yuv_img1[i];
			unsigned char tmpU1 = yuv_img1[W*H + i / 4];
			unsigned char tmpV1 = yuv_img1[W*H + W*H / 4 + i / 4];
			short Y1 = (short)tmpY1;
			short U1 = (short)tmpU1;
			short V1 = (short)tmpV1;

			unsigned char tmpY2 = yuv_img2[i];
			unsigned char tmpU2 = yuv_img2[W*H + i / 4];
			unsigned char tmpV2 = yuv_img2[W*H + W*H / 4 + i / 4];
			short Y2 = (short)tmpY2;
			short U2 = (short)tmpU2;
			short V2 = (short)tmpV2;

			short R1, G1, B1, R2, G2, B2, y, u, v, R, G, B;
			R1 = Y1 + ((360 * (V1 - 128)) / 256);
			G1 = Y1 - ((88 * (U1 - 128) + 184 * (V1 - 128)) / 256);
			B1 = Y1 + ((455 * (U1 - 128)) / 256);

			R2 = Y2 + ((360 * (V2 - 128)) / 256);
			G2 = Y2 - ((88 * (U2 - 128) + 184 * (V2 - 128)) / 256);
			B2 = Y2 + ((455 * (U2 - 128)) / 256);

			R1 = MIN(255, MAX(R1, 0));
			G1 = MIN(255, MAX(G1, 0));
			B1 = MIN(255, MAX(B1, 0));
			R2 = MIN(255, MAX(R2, 0));
			G2 = MIN(255, MAX(G2, 0));
			B2 = MIN(255, MAX(B2, 0));

			R = ((short)Alpha * R1 + (256-(short)Alpha)*R2)/ 256;
			G = ((short)Alpha * G1 + (256 - (short)Alpha)*G2) / 256;
			B = ((short)Alpha * B1 + (256 - (short)Alpha)*B2) / 256;

			y = ((66 * R + 129 * G + 25 * B + 128) / 256) + 16;
			u = ((-38 * R - 74 * G + 112 * B + 128) / 256) + 128;
			v = ((112 * R - 94 * G - 18 * B + 128) / 256) + 128;

			y = MIN(255, MAX(y, 0));
			u = MIN(255, MAX(u, 0));
			v = MIN(255, MAX(v, 0));

			new_yuv[i] = (char)y;
			new_yuv[W*H + i / 4] = (char)u;
			new_yuv[W*H + W*H / 4 + i / 4] = (char)v;

		}
		totalT += GetTickCount() - starttime;
		fff.seekp(0, ios::end);
		fff.write(new_yuv, img_size * sizeof(char));
	}
	cout << "Total time no simd for picture merging: " << totalT << "ms" << endl;
	return time;
}

void transpose_mmx(__m64 & A, __m64 & R, __m64 & G, __m64 & B)
{
	__m64 tmp1, tmp2, res1, res2, res3, res4;
	tmp1 = _m_punpcklwd(A, R);
	tmp2 = _m_punpcklwd(G, B);
	res1 = _m_punpckldq(tmp1, tmp2);
	res2 = _m_punpckhdq(tmp1, tmp2);
	tmp1 = _m_punpckhwd(A, R);
	tmp2 = _m_punpckhwd(G, B);
	res3 = _m_punpckldq(tmp1, tmp2);
	res4 = _m_punpckhdq(tmp1, tmp2);

	A = res1;
	R = res2;
	G = res3;
	B = res4;
	
	return;
}

void yuv2argb_mmx(char * yuv_img, char * argb_img, __m64 alp)
{

	int * dest = (int*)argb_img;

	for (int i = 0; i < W*H; i += 4)
	{
		char * yy = yuv_img + i;
		char * uu = yuv_img + W*H + i / 4;
		char * vv = yuv_img + W*H + W*H / 4 + i / 4;

		int * Yptr = (int*)yy;
		short * Uptr = (short*)uu;
		short * Vptr = (short*)vv;
		int tmpY = *(Yptr);
		short tmpU = *(Uptr);
		short tmpV = *(Vptr);


		__m64 Y = _m_from_int(tmpY);
		__m64 U = _m_from_int((int)tmpU);
		__m64 V = _m_from_int((int)tmpV);

		Y = _m_punpcklbw(Y, zero);
		U = _m_punpcklbw(U, U);
		U = _m_punpcklbw(U, zero);
		V = _m_punpcklbw(V, V);
		V = _m_punpcklbw(V, zero);

		__m64 R, G, B;

		R = computeRGB_mmx(Y, U, V, 0);
		G = computeRGB_mmx(Y, U, V, 1);
		B = computeRGB_mmx(Y, U, V, 2);
		
		*(dest++) = _m_to_int(alp);
		*(dest++) = _m_to_int(R);
		*(dest++) = _m_to_int(G);
		*(dest++) = _m_to_int(B);


	}
}

void yuv2argb_sse(int * yuv_img, __int64 * argb_img, __m128i alp)
{
	__m64 * dest = (__m64 *)argb_img;

	int tmpU, tmpV;
	for (int i = 0; i < W*H; i += 8)
	{
		int * Yptr = yuv_img + i / 4;
		int * Uptr = yuv_img + W*H / 4 + i / 16;
		int * Vptr = yuv_img + W*H / 4 + W*H / 16 + i / 16;
		

		__m64 tmpY1 = _m_from_int(*Yptr);
		__m64 tmpY2 = _m_from_int(*(Yptr + 1));
		__m64 tmpY = _m_punpckldq(tmpY1, tmpY2);

		int tmpU = *Uptr;
		int tmpV = *Vptr;

		__m128i Y, U, V;

		Y = _mm_set_epi64(zero, tmpY);
		Y = _mm_unpacklo_epi8(Y, zero_128);
		U = _mm_set_epi32(0, 0, tmpU, tmpU);
		U = _mm_unpacklo_epi8(U, zero_128);
		V = _mm_set_epi32(0, 0, tmpV, tmpV);
		V = _mm_unpacklo_epi8(V, zero_128);

		__m128i R, G, B;
		
		R = computeRGB_sse(Y, U, V, 0);
		G = computeRGB_sse(Y, U, V, 1);
		B = computeRGB_sse(Y, U, V, 2);

		*(dest++) = _mm_movepi64_pi64(alp);
		*(dest++) = _mm_movepi64_pi64(R);
		*(dest++) = _mm_movepi64_pi64(G);
		*(dest++) = _mm_movepi64_pi64(B);
	}
}

void yuv2argb_avx(int * yuv_img, __int64 * argb_img, __m256i alp)
{
	__m64 * dest = (__m64 *)argb_img;

	int tmpU, tmpV;
	for (int i = 0; i < W*H; i += 16)
	{
		int * Yptr = yuv_img + i / 4;
		int * Uptr = yuv_img + W*H / 4 + i / 16;
		int * Vptr = yuv_img + W*H / 4 + W*H / 16 + i / 16;

		int tmpU1 = *Uptr;
		int tmpU2 = *(Uptr + 1);
		int tmpV1 = *Vptr;
		int tmpV2 = *(Vptr + 1);
		
		__m256i Y, U, V;

		Y = _mm256_set_epi32(0, 0, *(Yptr + 3), *(Yptr + 2),0,0, *(Yptr + 1), *(Yptr));
		Y = _mm256_unpacklo_epi8(Y, zero_256);
		U = _mm256_set_epi32(0,0, tmpU2, tmpU2, 0,0,tmpU1, tmpU1);
		U = _mm256_unpacklo_epi8(U, zero_256);
		V = _mm256_set_epi32(0, 0,tmpV2, tmpV2, 0, 0,tmpV1,tmpV1);
		V = _mm256_unpacklo_epi8(V, zero_256);

		__m256i R, G, B;

		R = computeRGB_avx(Y, U, V, 0);
		G = computeRGB_avx(Y, U, V, 1);
		B = computeRGB_avx(Y, U, V, 2);

		// store lower 128 bit
		__m128i Rh = _mm256_extracti128_si256(R, 1);
		__m128i Rl = _mm256_extracti128_si256(R, 0);
		__m128i Gh = _mm256_extracti128_si256(G, 1);
		__m128i Gl = _mm256_extracti128_si256(G, 0);
		__m128i Bh = _mm256_extracti128_si256(B, 1);
		__m128i Bl = _mm256_extracti128_si256(B, 0);

		__m128i alph = _mm256_extracti128_si256(alp, 1);
		__m128i alpl = _mm256_extracti128_si256(alp, 0);

		*(dest++) = _mm_movepi64_pi64(alpl);
		*(dest++) = _mm_movepi64_pi64(alph);
		*(dest++) = _mm_movepi64_pi64(Rl);
		*(dest++) = _mm_movepi64_pi64(Rh);
		*(dest++) = _mm_movepi64_pi64(Gl);
		*(dest++) = _mm_movepi64_pi64(Gh);
		*(dest++) = _mm_movepi64_pi64(Bl);
		*(dest++) = _mm_movepi64_pi64(Bh);
	}
}

void rgb2yuv_picture_merge_mmx(char * yuv_img, char * argb_img1, char * argb_img2)
{
	int * src1 = (int*)argb_img1;
	int * src2 = (int*)argb_img2;

	for (int i = 0; i < W*H; i += 4)
	{
		char * yy = yuv_img + i;
		char * uu = yuv_img + W*H + i / 4;
		char * vv = yuv_img + W*H + W*H / 4 + i / 4;
		int * Yptr = (int*)yy;
		short * Uptr = (short*)uu;
		short * Vptr = (short*)vv;
		__m64 A1, R1, G1, B1, A2, R2, G2, B2;

		zero = _m_pxor(zero, zero);
		A1 = _m_from_int(*(src1++));
		A2 = _m_from_int(*(src2++));
		R1 = _m_from_int(*(src1++));
		R2 = _m_from_int(*(src2++));
		G1 = _m_from_int(*(src1++));
		G2 = _m_from_int(*(src2++));
		B1 = _m_from_int(*(src1++));
		B2 = _m_from_int(*(src2++));
		A1 = _m_punpcklbw(A1, zero);
		A2 = _m_punpcklbw(A1, zero);
		R1 = _m_punpcklbw(R1, zero);
		R2 = _m_punpcklbw(R2, zero);
		G1 = _m_punpcklbw(G1, zero);
		G2 = _m_punpcklbw(G2, zero);
		B1 = _m_punpcklbw(B1, zero);
		B2 = _m_punpcklbw(B2, zero);

		__m64 R, G, B;


		R = picture_merge_mmx(A1, R1, R2);
		G = picture_merge_mmx(A1, G1, G2);
		B = picture_merge_mmx(A1, B1, B2);
		
		__m64 Y, U, V;
		
		Y = computeYUV_mmx(R, G, B, 0);
		U = computeYUV_mmx(R, G, B, 1);
		V = computeYUV_mmx(R, G, B, 2);

		char tmpU[4] = { 0 };
		char tmpV[4] = { 0 };
		*((int*)tmpU) = _m_to_int(U);
		*((int*)tmpV) = _m_to_int(V);
		*(Uptr) = tmpU[0];
		*(Uptr + 1) = tmpU[1];
		*(Vptr) = tmpV[0];
		*(Vptr + 1) = tmpV[1];
		*(Yptr) = _m_to_int(Y);

	}

}

void rgb2yuv_picture_merge_sse(int * yuv_img, __int64 * argb_img1, __int64 * argb_img2)
{
	__int64 * src1 = argb_img1;
	__int64 * src2 = argb_img2;

	for (int i = 0; i < W*H; i += 8)
	{
		int * Yptr = yuv_img + i / 4;
		int * Uptr = yuv_img + W*H / 4 + i / 16;
		int * Vptr = yuv_img + W*H / 4 + W*H / 16 + i / 16;

		__m64 * Yptr1 = (__m64*)Yptr;

		__m128i A1, R1, G1, B1, A2, R2, G2, B2;

		A1 = _mm_set_epi64x(0, *(src1++));
		A2 = _mm_set_epi64x(0, *(src2++));
		R1 = _mm_set_epi64x(0, *(src1++));
		R2 = _mm_set_epi64x(0, *(src2++));
		G1 = _mm_set_epi64x(0, *(src1++));
		G2 = _mm_set_epi64x(0, *(src2++));
		B1 = _mm_set_epi64x(0, *(src1++));
		B2 = _mm_set_epi64x(0, *(src2++));
		A1 = _mm_unpacklo_epi8(A1, zero_128);
		A2 = _mm_unpacklo_epi8(A1, zero_128);
		R1 = _mm_unpacklo_epi8(R1, zero_128);
		R2 = _mm_unpacklo_epi8(R2, zero_128);
		G1 = _mm_unpacklo_epi8(G1, zero_128);
		G2 = _mm_unpacklo_epi8(G2, zero_128);
		B1 = _mm_unpacklo_epi8(B1, zero_128);
		B2 = _mm_unpacklo_epi8(B2, zero_128);

		__m128i R, G, B;

		R = picture_merge_sse(A1, R1, R2);
		G = picture_merge_sse(A1, G1, G2);
		B = picture_merge_sse(A1, B1, B2);

		__m128i Y, U, V;

		Y = computeYUV_sse(R, G, B, 0);
		U = computeYUV_sse(R, G, B, 1);
		V = computeYUV_sse(R, G, B, 2);
		
		*(Yptr1) = _mm_movepi64_pi64(Y);
		*(Uptr) = _mm_cvtsi128_si32(U);
		*(Vptr) = _mm_cvtsi128_si32(V);
	}
}

void rgb2yuv_picture_merge_avx(int * yuv_img, __int64 * argb_img1, __int64 * argb_img2)
{
	__int64 * src1 = argb_img1;
	__int64 * src2 = argb_img2;

	for (int i = 0; i < W*H; i += 16)
	{
		int * Yptr = yuv_img + i / 4;
		int * Uptr = yuv_img + W*H / 4 + i / 16;
		int * Vptr = yuv_img + W*H / 4 + W*H / 16 + i / 16;


		__m256i A1, R1, G1, B1, A2, R2, G2, B2;
		__m256i shamtmask = _mm256_set_epi64x(0, 0xffffffffffffffff, 0, 0);
		A1 = _mm256_set_epi64x(0, *(src1 + 1),0,*(src1));
		A2 = _mm256_set_epi64x(0, *(src2 + 1),0,*(src2));
		R1 = _mm256_set_epi64x(0, *(src1+3), 0, *(src1+2));
		R2 = _mm256_set_epi64x(0, *(src2+3), 0, *(src2+2));
		G1 = _mm256_set_epi64x(0, *(src1+5), 0, *(src1+4));
		G2 = _mm256_set_epi64x(0, *(src2+5), 0, *(src2+4));
		B1 = _mm256_set_epi64x(0, *(src1+7), 0, *(src1+6));
		B2 = _mm256_set_epi64x(0, *(src2+7), 0, *(src2+6));
		A1 = _mm256_unpacklo_epi8(A1, zero_256);
		A2 = _mm256_unpacklo_epi8(A1, zero_256);
		R1 = _mm256_unpacklo_epi8(R1, zero_256);
		R2 = _mm256_unpacklo_epi8(R2, zero_256);
		G1 = _mm256_unpacklo_epi8(G1, zero_256);
		G2 = _mm256_unpacklo_epi8(G2, zero_256);
		B1 = _mm256_unpacklo_epi8(B1, zero_256);
		B2 = _mm256_unpacklo_epi8(B2, zero_256);
		src1 += 8;
		src2 += 8;

		__m256i R, G, B;

		R = picture_merge_avx(A1, R1, R2);
		G = picture_merge_avx(A1, G1, G2);
		B = picture_merge_avx(A1, B1, B2);

		__m256i Y, U, V;

		Y = computeYUV_avx(R, G, B, 0);
		U = computeYUV_avx(R, G, B, 1);
		V = computeYUV_avx(R, G, B, 2);

		__m256i mask2 = _mm256_set_epi64x(0, 0, 0, 0xffffffffffffffff);
		__m128i Yh, Yl;
		Yh = _mm256_extracti128_si256(Y, 1);
		Yl = _mm256_extracti128_si256(Y, 0);
		__m128i mask3 = _mm_set_epi32(0, 0, 0xffffffff, 0xffffffff);
		_mm_maskstore_epi32(Yptr, mask3, Yl);
		_mm_maskstore_epi32(Yptr + 2, mask3, Yh);
		_mm256_maskstore_epi32(Uptr, mask2, U);
		_mm256_maskstore_epi32(Vptr, mask2, V);

	}
}

void rgb2yuv_fade_mmx(char * yuv_img, char*argb_img)
{
	int * src = (int*)argb_img;

	for (int i = 0; i < W*H; i += 4)
	{
		char * yy = yuv_img + i;
		char * uu = yuv_img + W*H + i / 4;
		char * vv = yuv_img + W*H + W*H / 4 + i / 4;
		int * Yptr = (int*)yy;
		short * Uptr = (short*)uu;
		short * Vptr = (short*)vv;
		__m64 A, R, G, B;

		A = _m_from_int(*(src++));
		R = _m_from_int(*(src++));
		G = _m_from_int(*(src++));
		B = _m_from_int(*(src++));
		A = _m_punpcklbw(A, zero);
		R = _m_punpcklbw(R, zero);
		G = _m_punpcklbw(G, zero);
		B = _m_punpcklbw(B, zero);


		R = fade_mmx(A, R);
		G = fade_mmx(A, G);
		B = fade_mmx(A, B);

		__m64 Y, U, V;

		Y = computeYUV_mmx(R, G, B, 0);
		U = computeYUV_mmx(R, G, B, 1);
		V = computeYUV_mmx(R, G, B, 2);

		char tmpU[4] = { 0 };
		char tmpV[4] = { 0 };
		*((int*)tmpU) = _m_to_int(U);
		*((int*)tmpV) = _m_to_int(V);
		*(Uptr) = tmpU[0];
		*(Uptr + 1) = tmpU[1];
		*(Vptr) = tmpV[0];
		*(Vptr + 1) = tmpV[1];
		*(Yptr) = _m_to_int(Y);

	}
}

void rgb2yuv_fade_sse(int * yuv_img, __int64* argb_img)
{
	__int64 * src = (__int64*)argb_img;

	for (int i = 0; i < W*H; i += 8)
	{
		int * Yptr = yuv_img + i / 4;
		int * Uptr = yuv_img + W*H / 4 + i / 16;
		int * Vptr = yuv_img + W*H / 4 + W*H / 16 + i / 16;

		__m64 * Yptr1 = (__m64*)Yptr;

		__m128i A, R, G, B;

		A = _mm_set_epi64x(0, *(src++));
		R = _mm_set_epi64x(0, *(src++));
		G = _mm_set_epi64x(0, *(src++));
		B = _mm_set_epi64x(0, *(src++));
		A = _mm_unpacklo_epi8(A, zero_128);
		R = _mm_unpacklo_epi8(R, zero_128);
		G = _mm_unpacklo_epi8(G, zero_128);
		B = _mm_unpacklo_epi8(B, zero_128);


		R = fade_sse(A, R);
		G = fade_sse(A, G);
		B = fade_sse(A, B);

		__m128i Y, U, V;

		Y = computeYUV_sse(R, G, B, 0);
		U = computeYUV_sse(R, G, B, 1);
		V = computeYUV_sse(R, G, B, 2);

		*(Yptr1) = _mm_movepi64_pi64(Y);
		*(Uptr) = _mm_cvtsi128_si32(U);
		*(Vptr) = _mm_cvtsi128_si32(V);
	}
}

void rgb2yuv_fade_avx(int * yuv_img, __int64*argb_img)
{
	__int64 * src = argb_img;

	for (int i = 0; i < W*H; i += 16)
	{
		int * Yptr = yuv_img + i / 4;
		int * Uptr = yuv_img + W*H / 4 + i / 16;
		int * Vptr = yuv_img + W*H / 4 + W*H / 16 + i / 16;


		__m256i A, R, G, B;
		__m256i shamtmask = _mm256_set_epi64x(0, 0xffffffffffffffff, 0, 0);
		A = _mm256_set_epi64x(0, *(src + 1), 0, *(src));
		R = _mm256_set_epi64x(0, *(src + 3), 0, *(src + 2));
		G = _mm256_set_epi64x(0, *(src + 5), 0, *(src + 4));
		B = _mm256_set_epi64x(0, *(src + 7), 0, *(src + 6));
		A = _mm256_unpacklo_epi8(A, zero_256);
		R = _mm256_unpacklo_epi8(R, zero_256);
		G = _mm256_unpacklo_epi8(G, zero_256);
		B = _mm256_unpacklo_epi8(B, zero_256);
		src += 8;


		R = fade_avx(A,R);
		G = fade_avx(A, G);
		B = fade_avx(A, B);

		__m256i Y, U, V;

		Y = computeYUV_avx(R, G, B, 0);
		U = computeYUV_avx(R, G, B, 1);
		V = computeYUV_avx(R, G, B, 2);

		__m256i mask2 = _mm256_set_epi64x(0, 0, 0, 0xffffffffffffffff);
		__m128i Yh, Yl;
		Yh = _mm256_extracti128_si256(Y, 1);
		Yl = _mm256_extracti128_si256(Y, 0);
		__m128i mask3 = _mm_set_epi32(0, 0, 0xffffffff, 0xffffffff);
		_mm_maskstore_epi32(Yptr, mask3, Yl);
		_mm_maskstore_epi32(Yptr + 2, mask3, Yh);
		_mm256_maskstore_epi32(Uptr, mask2, U);
		_mm256_maskstore_epi32(Vptr, mask2, V);

	}
}

int process_with_mmx_picture_merge(char *yuv_img1, char*yuv_img2)
{
	int time = 0;
	string filename = "./part2/mmx/figure.yuv";
	ofstream yuvout(filename.c_str(), ios::out);

    DWORD totalT = 0;
	char * argb1 = (char*)malloc(W*H * 4 * sizeof(char));
	char * argb2 = (char*)malloc(W*H * 4 * sizeof(char));
	char * new_yuv = (char*)malloc(img_size * sizeof(char));
	for (int Alpha = 1 ; Alpha <= 255; Alpha += 3)
	{
		DWORD starttime = GetTickCount();
		__m64 alp = _m_from_int(Alpha);
		alp = _m_punpcklbw(alp, alp);
		alp = _m_punpcklwd(alp, alp);
		// yuv->argb
		yuv2argb_mmx(yuv_img1, argb1, alp);
		yuv2argb_mmx(yuv_img2, argb2, alp);

		rgb2yuv_picture_merge_mmx(new_yuv, argb1, argb2);

		totalT += GetTickCount() - starttime;
		yuvout.seekp(0, ios::end);
		yuvout.write(new_yuv, img_size * sizeof(char));
	}
	cout << "Total time MMX for picture merging: " << totalT  << "ms" << endl;
	return time;
}

int process_with_mmx_fade(char * yuv_img)
{
	int time = 0;
	string filename = "./part1/mmx/figure.yuv";
	ofstream yuvout(filename.c_str(), ios::out);

    DWORD totalT = 0;
	char * argb = (char*)malloc(W*H * 4 * sizeof(char));
	char * new_yuv = (char*)malloc(img_size * sizeof(char));
	for (int Alpha = 1; Alpha <= 255; Alpha += 3)
	{
		DWORD starttime = GetTickCount();
		__m64 alp = _m_from_int(Alpha);
		alp = _m_punpcklbw(alp, alp);
		alp = _m_punpcklwd(alp, alp);
		// yuv->argb
		yuv2argb_mmx(yuv_img, argb, alp);

		rgb2yuv_fade_mmx(new_yuv, argb);

		totalT += GetTickCount() - starttime;
		yuvout.seekp(0, ios::end);
		yuvout.write(new_yuv, img_size * sizeof(char));
	}
	cout << "Total time MMX for fade in fade out: " << totalT << "ms" << endl;
	return time;
}

int process_with_sse_picture_merge(int*yuv_img1, int*yuv_img2) {
	int time = 0;
	string filename = "./part2/sse2/figure.yuv";
	ofstream yuvout(filename.c_str(), ios::out);
	DWORD totalT = 0;
	__int64 * argb1 = (__int64*)malloc(W*H*4 * sizeof(char));
	__int64 * argb2 = (__int64*)malloc(W*H * 4 * sizeof(char));
	int * new_yuv = (int*)malloc(img_size * sizeof(char));
	char * write_yuv = (char*)malloc(img_size * sizeof(char));
	for (int Alpha = 1; Alpha <= 255; Alpha += 3)
	{
		DWORD starttime = GetTickCount();
		__m128i alp = _mm_set1_epi8((char)Alpha);
		// yuv->argb

		yuv2argb_sse(yuv_img1, argb1, alp);
		yuv2argb_sse(yuv_img2, argb2, alp);

		rgb2yuv_picture_merge_sse(new_yuv, argb1, argb2);

		totalT += GetTickCount() - starttime;


		memcpy(write_yuv, new_yuv, img_size * sizeof(char));
		
		yuvout.seekp(0, ios::end);
		yuvout.write(write_yuv, img_size * sizeof(char));
	}
	cout << "Total time SSE for picture merging: " << totalT << "ms" << endl;

	return time;
}

int process_with_sse_fade(int*yuv_img)
{
	int time = 0;
	string filename = "./part1/sse2/figure.yuv";
	ofstream yuvout(filename.c_str(), ios::out);
	DWORD totalT = 0;
	__int64 * argb = (__int64*)malloc(W*H * 4 * sizeof(char));
	int * new_yuv = (int*)malloc(img_size * sizeof(char));
	char * write_yuv = (char*)malloc(img_size * sizeof(char));
	for (int Alpha = 1; Alpha <= 255; Alpha += 3)
	{
		DWORD starttime = GetTickCount();
		__m128i alp = _mm_set1_epi8((char)Alpha);
		// yuv->argb

		yuv2argb_sse(yuv_img, argb, alp);

		rgb2yuv_fade_sse(new_yuv, argb);

		totalT += GetTickCount() - starttime;

		memcpy(write_yuv, new_yuv, img_size * sizeof(char));
		yuvout.seekp(0, ios::end);
		yuvout.write(write_yuv, img_size * sizeof(char));
	}
	cout << "Total time SSE for fade in fade out: " << totalT << "ms" << endl;

	return time;
}

int process_with_avx_picture_merge(int*yuv_img1, int*yuv_img2)
{
	int time = 0;
	string filename = "./part2/avx/figure.yuv";
	ofstream yuvout(filename.c_str(), ios::out);
	DWORD totalT = 0;
	__int64 * argb1 = (__int64*)malloc(W*H * 4 * sizeof(char));
	__int64 * argb2 = (__int64*)malloc(W*H * 4 * sizeof(char));
	int * new_yuv = (int*)malloc(img_size * sizeof(char));
	char * write_yuv = (char*)malloc(img_size * sizeof(char));
	for (int Alpha = 1; Alpha <= 255; Alpha += 3)
	{
		DWORD starttime = GetTickCount();
		__m256i alp = _mm256_set1_epi8((char)Alpha);
		// yuv->argb

		yuv2argb_avx(yuv_img1, argb1, alp);
		yuv2argb_avx(yuv_img2, argb2, alp);

		rgb2yuv_picture_merge_avx(new_yuv, argb1, argb2);

		totalT += GetTickCount() - starttime;

		memcpy(write_yuv, new_yuv, img_size * sizeof(char));
		yuvout.seekp(0, ios::end);
		yuvout.write(write_yuv, img_size * sizeof(char));
	}
	cout << "Total time AVX for picture merging: " << totalT << "ms" << endl;
	return time;
}

int process_with_avx_fade(int * yuv_img)
{
	int time = 0;

	string filename = "./part1/avx/figure.yuv";
	ofstream yuvout(filename.c_str(), ios::out);
	DWORD totalT = 0;
	__int64 * argb = (__int64*)malloc(W*H * 4 * sizeof(char));
	int * new_yuv = (int*)malloc(img_size * sizeof(char));
	char * write_yuv = (char*)malloc(img_size * sizeof(char));
	for (int Alpha = 1; Alpha <= 255; Alpha += 3)
	{
		DWORD starttime = GetTickCount();
		__m256i alp = _mm256_set1_epi8((char)Alpha);
		// yuv->argb

		yuv2argb_avx(yuv_img, argb, alp);

		rgb2yuv_fade_avx(new_yuv, argb);

		totalT += GetTickCount() - starttime;

		memcpy(write_yuv, new_yuv, img_size * sizeof(char));
		yuvout.seekp(0, ios::end);
		yuvout.write(write_yuv, img_size * sizeof(char));
	}
	cout << "Total time AVX for fade in fade out: " << totalT<< "ms" << endl;

	return time;
}

