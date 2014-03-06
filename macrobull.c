/* Blink LED example */
 
#include <msp430.h>

#define _DEV_LAUNCHPAD

#define PENA	BIT7
#define PEV	BIT6

#define SPB	BIT5
#define CHAR	BIT4

#define Tus 8


 
delay10us(unsigned int d) {
  while (d--){
	  __delay_cycles(Tus-1);
  }
}

delayms(unsigned int d) {
  while (d--){
	  //for (j=0;j<100;j++) nop();
	  __delay_cycles(800);
  }
}

int atoi(char s[]){
	char neg, v;
	neg=(*(s++)=='-');
	v=0;
	while (*s){
		v*=10;
		v+=*s-48;
		s++;
	}
	return v | (neg <<15);
}
char t_status;
unsigned int t_tmp, t_0, t_1;

inline unsigned int time(){
	#define TIME_OFFSET_DEFAULT 56
	#define TIME_OFFSET_O2 30
	#define TIME_OFFSET_O2_INLINE 17
	
	#ifdef __OPTIMIZE__
		#define TIME_OFFSET TIME_OFFSET_O2_INLINE
	#else
		#define TIME_OFFSET TIME_OFFSET_DEFAULT
	#endif
	
	t_tmp = TAR;
	if (t_status==0) t_0 = t_tmp;
	if (t_status==1) t_1 = t_tmp;
	t_status +=1;
	if (t_status==2) {
		t_status = 0;
		return t_1 - t_0 - TIME_OFFSET;
	}
	else return 0;
}