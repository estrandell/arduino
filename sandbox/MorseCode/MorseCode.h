#define SHORT 0
#define LONG 1

#define MORSE1(a0)				{ 2,a0				}
#define MORSE2(a0,a1)			{ 3,a0,a1			}
#define MORSE3(a0,a1,a2)		{ 4,a0,a1,a2		}
#define MORSE4(a0,a1,a2,a3)		{ 5,a0,a1,a2,a3		}
#define MORSE5(a0,a1,a2,a3,a4)	{ 6,a0,a1,a2,a3,a4	}

#define Morse_A MORSE2(SHORT, LONG);
#define Morse_B MORSE4(LONG, SHORT, SHORT, SHORT);
#define Morse_C MORSE4(LONG, SHORT, LONG, SHORT);
#define Morse_D MORSE3(LONG, SHORT, SHORT);
#define Morse_E MORSE1(SHORT);
#define Morse_F MORSE4(SHORT, SHORT, LONG, SHORT);
#define Morse_G MORSE3(LONG, LONG, SHORT);
#define Morse_H MORSE4(SHORT, SHORT, SHORT, SHORT);
#define Morse_I MORSE2(SHORT, SHORT);
#define Morse_J MORSE4(SHORT, LONG, LONG, LONG);
#define Morse_K MORSE3(LONG, SHORT, LONG);
#define Morse_L MORSE4(SHORT, LONG, SHORT, SHORT);
#define Morse_M MORSE2(LONG, LONG);
#define Morse_N MORSE2(LONG, SHORT);
#define Morse_O MORSE3(LONG, LONG, LONG);
#define Morse_P MORSE4(SHORT, LONG, LONG, SHORT);
#define Morse_Q MORSE4(LONG, LONG, SHORT, LONG);
#define Morse_R MORSE3(SHORT, LONG, SHORT);
#define Morse_S MORSE3(SHORT, SHORT, SHORT);
#define Morse_T MORSE1(LONG);
#define Morse_U MORSE3(SHORT, SHORT, LONG);
#define Morse_V MORSE4(SHORT, SHORT, SHORT, LONG);
#define Morse_W MORSE3(SHORT, LONG, LONG);
#define Morse_X MORSE4(LONG, SHORT, SHORT, LONG);
#define Morse_Y MORSE4(LONG, SHORT, LONG, LONG);
#define Morse_Z MORSE4(LONG, LONG, SHORT, SHORT);

#define Morse_1 MORSE5(SHORT, LONG, LONG, LONG, LONG);
#define Morse_2 MORSE5(SHORT, SHORT, LONG, LONG, LONG);
#define Morse_3 MORSE5(SHORT, SHORT, SHORT, LONG, LONG);
#define Morse_4 MORSE5(SHORT, SHORT, SHORT, SHORT, LONG);
#define Morse_5 MORSE5(SHORT, SHORT, SHORT, SHORT, SHORT);
#define Morse_6 MORSE5(LONG, SHORT, SHORT, SHORT, SHORT);
#define Morse_7 MORSE5(LONG, LONG, SHORT, SHORT, SHORT);
#define Morse_8 MORSE5(LONG, LONG, LONG, SHORT, SHORT);
#define Morse_9 MORSE5(LONG, LONG, LONG, LONG, SHORT);
#define Morse_0 MORSE5(LONG, LONG, LONG, LONG, LONG);

// ie:
// std::vector<int> v = Morse(D);
// int a[] = Morse(K);
#define Morse(C) Morse_##C 



