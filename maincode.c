#include      "dsk6713_aic23.h"                  //codec      support
Uint32 fs=DSK6713_AIC23_FREQ_8KHZ;  //set sampling rate
#define DSK6713_AIC23_INPUT_MIC 0x0015
#define DSK6713_AIC23_INPUT_LINE 0x0011
Uint16 inputsource=DSK6713_AIC23_INPUT_LINE; //select input

#include <math.h>
#define N 256                       //no of complex FFT points

// for signal generation
#define LOOPLENGTH 8

short loopindex = 0;         	      //table index
short gain = 10;			          //gain factor
short sine_table[LOOPLENGTH]={0,707,1000,707,0,-707,-1000,-707}; //sine values

unsigned short JIndex[4*N];         //index for digit reversal
unsigned short IIndex[4*N];         //index for digit reversal
int i, count;
float Xmag[N];                      //magnitude spectrum of x
float fakeInput;

typedef struct Complex_tag {float re,im;}Complex;
Complex W[3*N/2];                   //array for twiddle constants
Complex x[N];                       //N complex data values
double delta = 2*3.14159265359/N;

#pragma DATA_ALIGN(x,sizeof(Complex)); //align x on boundary
#pragma DATA_ALIGN(W,sizeof(Complex)); //align W on boundary

void main()
{
	R4DigitRevIndexTableGen(N,&count,IIndex,JIndex); //for digit rev

	for(i = 0; i < 3*N/4; i++)
  	{
   		W[i].re = cos(delta*i);          //real component of W
  		W[i].im = sin(delta*i);          //Im component of W
  	}

	comm_poll();                       //init DSK, codec, McBSP
	DSK6713_LED_init();			      //init LED from BSL
  	DSK6713_DIP_init();			      //init DIP from BSL

 	for(i=0; i<N; i++)
 		Xmag[i] = 0;                       //init output magnitude

	while (1)                          //infinite loop
	{
		output_left_sample(32000);        //-ve spike for reference

		if(DSK6713_DIP_get(0)==0) 		  //=0 if DIP switch #0 pressed
	    {
	    	DSK6713_LED_on();	   		      //turn LED #0 ON
	    	fakeInput = sine_table[loopindex++]*gain; //output sample
	    	if (loopindex >= LOOPLENGTH) loopindex = 0; //reset table index
	    }
		else
		{
			fakeInput = 0;
			DSK6713_LED_off(0);
		}

		for( i = 0 ; i < N ; i++ )
		{
			//x[i].re = (float)((short)input_left_sample()); //get input
			x[i].re = fakeInput; //get input
			x[i].im = 0.0;                   //zero imaginary part
			if(i>0) output_left_sample((short)Xmag[i]);//output magnitude
		}

		cfftr4_dif(x, W, N);              //radix-4 FFT function
		digit_reverse((double *)x,IIndex,JIndex,count);//unscramble

		for (i =0; i<N; i++)
			Xmag[i] = -sqrt(x[i].re*x[i].re+x[i].im*x[i].im)/32; //mag X
	}
}
