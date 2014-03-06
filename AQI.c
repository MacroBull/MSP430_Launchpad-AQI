#include <msp430.h>

//#define FAST_SWITCH

#include "dev.h"
#include "uart_legacy.h"
#include "adc10.h"
#include "misc.h"

#ifdef debug
	//#define debugLen
	//#define debugTIME
	#define debugData
#else
	//#define WDT_ENABLE

#endif

#define AI 5
#define AT 10
#define AHV 11 // half VCC

#define INCH_AI INCH_5 // input
#define INCH_AHV INCH_11 // half VCC

#define ADCLED BIT6
#define SENLED BIT4

#define MAXLEN 20

#define T_ASS_DEFAULT 7
#define T_PRELOAD_DEFAULT	(75+40)  // 72 ADCon, 22.5 ADC preSample, true is 22.5 + 13.5c
#define T_ASS_O2 6
#define T_PRELOAD_O2	(30+30)  // 72 ADCon, 22.5 ADC preSample, true is 22.5 + 13.5c

#ifdef __OPTIMIZE__
	#define T_ASS T_ASS_O2
	#define T_PRELOAD	T_PRELOAD_O2
#else
	#define T_ASS T_ASS_DEFAULT
	#define T_PRELOAD	T_PRELOAD_DEFAULT
#endif

#define T_DELAY		(280-T_ASS-T_PRELOAD) //0.28ms, status1
#define T_SAMPLE	(40-T_ASS+T_PRELOAD) //0.04ms, status2
#define T_WAIT 		(9680-T_ASS) //+=10ms, status0 
#define T_LWAIT 	(50000-T_ASS) //50ms * 10   <0xffff

#define MAXSTAT 200

#ifdef debugTIME
unsigned int t0, t1, t2, t3, t4;
#endif

unsigned char len, status, source;//rxAcc; 
unsigned int data[MAXLEN]; 

int ADC_Done(){
	P1OUT &= ~ADCLED;
	if (status == 2) { // before turn off
		status = 3; //ADC10 idle
		ADC10CTL0 &= ~ENC; // 9 CLKs
		//__bic_SR_register_on_exit(CPUOFF); //got valid result
		return -CPUOFF;
	}
	else return 0;
}

void initTimer(){
	//Using SM = 1MHz
	
	TACTL = TASSEL_2 + MC_2; //continuous mode
	TACCR0 = T_LWAIT;                 
	TACCTL0 |= CCIE;  
	
}

void main(void)

{
	unsigned int vcc_data;
	unsigned long int result;
	
	WDT_disable; // stop WDT
	
	BC1MSET; // set SMCLK to 1MHZ
	UART0_XLED_enable;
	UART0_init(1000000, 9600, 0); //baud = 9600
	
	P1DIR |= ADCLED + SENLED; // adc indicator and sensor LED
	len=7;//MAXLEN;
	//rxAcc = 0;
	P1OUT= SENLED; //High for off
	
#ifdef FAST_SWITCH
	ADC10_init(AHV, 0, 25, 16, 0, 2, 1); // first for REFON, REF2_5V
	ADC10AE0 = 1 << AI;
#endif
	
	initTimer();
	status = 4; // status : done
	source = 0;
	while (1){	
		
#ifdef FAST_SWITCH
		if (source) {
			ADC10CTL1 &= ~INCH_15;
			ADC10CTL1 |= INCH_AI;// INCH_AI
			ADC10CTL0 &= SREF_3;// REF=Vcc
			//ADC10AE0 = 32;
		}
		else {
			ADC10CTL1 &= ~INCH_15;
			ADC10CTL1 |= INCH_AHV;// INCH_AHV
			ADC10CTL0 &= SREF_3;
			ADC10CTL0 |= SREF0;// REF=REF2_5
		}
		ljustInt(ADC10CTL0, 8);
		ljustInt(ADC10AE0, 8);
#else
		if (source)
			ADC10_init(AI, 0, 50, 64, 0, 2, ADC_Done); // read data from AI
		else
			ADC10_init(AHV, 0, 25, 64, 0, 2, ADC_Done); // read data from (Vss+Vcc)/2
		//ADC10AE0 = 1 << AI;
#endif
		__bis_SR_register(CPUOFF);
				
#ifdef debug
	#ifdef debugData
		printStr("________________");
		int i;
		for (i=0;i<len;i++) {
			ljustInt(i,8);
			ljustInt(data[i],8);
		}
	#endif
	#ifdef debugTIME
		ljustInt(status, 8);
		ljustInt(t1-t0, 8);
		ljustInt(len, 8);
		printStr("________");
	#endif
#else
		avgFilter(data, len, 1);
		/*
		 * 0) 2.5 * data0 / 0x400 == Vcc /2
		 * 1) v = Vcc * data1 / 0x400 = 5 / 0x100000 * data0 * data1
		 */
		if (source){
			result = data[0];
			result = (result * vcc_data * 5000 + 0x80000) >> 20;
			ljustInt(result, 8); // ADC voltage
		}
		else {
			vcc_data = data[0]; 
			result = vcc_data;
			result = (result * 5000 + 0x200) / 0x400;
			printStr("V=");
			ljustInt(result, 6); // Vcc voltage
		}
#endif
		source ^=1; // swtich between sensor/Vcc
		//weego;
		
	}
}


#pragma vector=TIMER0_A0_VECTOR
__interrupt void timeup(void){
	
	switch (status){
		case 0: // turn on sensor LED
			TACCR0 += T_DELAY;
			status = 1;
			//P1OUT |= ADCLED;
			P1OUT &= ~SENLED; 
			break;
		case 1: // Start ADC
#ifdef debugTIME
			t0=TAR;
#endif
			TACCR0 += T_SAMPLE;
			status = 2; // ADC10 really busy
			P1OUT |= ADCLED;
			ADC10_startMSC(data,len);
			
#ifdef debugTIME
			t1=TAR;
#endif
			break;
		case 2: // ADC is late.
			len-=1;
		case 3: // turn off sensor LED
			//P1OUT &= ~ADCLED;
			TACCR0 += T_WAIT;
			status = 4;
			P1OUT |= SENLED;
			break;
		default: // looooooong wait...
			if (status >= source * MAXSTAT) status = 0;
			else {
				TACCR0 += T_LWAIT;
				status += 1;
			}
			break;
	}
	
}
