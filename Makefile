CC = gcc
CFLAGS= -Wall -g

default: roll adc

roll: Rock_n_Roll.c
	$(CC) $(CFLAGS) -o roll Rock_n_Roll.c -lrt

adc: ADC_Setup.c
	$(CC) $(CFLAGS) -o adc ADC_Setup.c

clean: FORCE
	/bin/rm -f *.o *.a *~ adc roll

FORCE: