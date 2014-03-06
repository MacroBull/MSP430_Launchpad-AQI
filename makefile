####
TARGET = msp430g2553
BOARD = LAUNCHPAD
CC = msp430-gcc
CFLAGS = -mmcu=$(TARGET) -lm -D "_DEV_$(BOARD)" -g
CFLAGS_SIM = -mmcu=msp430f2131
VPATH = ../include

OSC_OBJ = oscilloscope_2ch_conseq2.o uart.o adc.o
AQI_OBJ = AQI.o uart_legacy.o adc10.o

main: clean clear aqi

flash: aqi.elf
	mspdebug rf2500 "prog aqi.elf"

osc: $(OSC_OBJ)
	$(CC) -o osc.elf $(CFLAGS) $(OSC_OBJ)
	
aqi: $(AQI_OBJ)
	$(CC) -o aqi.elf $(CFLAGS) $(AQI_OBJ)
	

%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clear:
	clear

clean:
	-rm *.elf *.o
	#-rm $(VPATH)/*.o
