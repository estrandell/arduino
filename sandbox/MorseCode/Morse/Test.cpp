// Morse.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <vector>
#include "../MorseCode.h"

using namespace std;

#define MorseVec(C,v) { int myarr[] = Morse_##C; v = new vector<int>(myarr, myarr+myarr[0]); } 

int _tmain(int argc, _TCHAR* argv[])
{
	vector<int> * vvv = NULL;

	std::vector<int> vvefwf = Morse(D);



	for (int i = 0; i < vvv->size(); i++)
		cout << vvv->at(i) << endl;

	int a[] = Morse_A;
	int b[] = Morse_B;
	int c[] = Morse_C;
	int d[] = Morse_D;
	int e[] = Morse_E;
	int f[] = Morse_F;
	int g[] = Morse_G;
	int h[] = Morse_H;
	int i[] = Morse_I;
	int j[] = Morse_J;
	int k[] = Morse_K;
	int l[] = Morse_L;
	int m[] = Morse_M;
	int n[] = Morse_N;
	int o[] = Morse_O;
	int p[] = Morse_P;
	int q[] = Morse_Q;
	int r[] = Morse_R;
	int s[] = Morse_S;
	int t[] = Morse_T;
	int u[] = Morse_U;
	int v[] = Morse_V;
	int w[] = Morse_W;
	int x[] = Morse_X;
	int y[] = Morse_Y;
	int z[] = Morse_Z;


	cout << "hej" << endl;

	int iii;
	cin >> iii;
	return 0;
}

