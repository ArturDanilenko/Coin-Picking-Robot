#include "lpc824.h"
#include "serial.h"

#define F_CPU 60000000L

void wait_1ms(void)
{
	// For SysTick info check the LPC824 manual page 317 in chapter 20.
	SYST_RVR = (F_CPU/1000L) - 1;  // set reload register, counter rolls over from zero, hence -1
	SYST_CVR = 0; // load the SysTick counter
	SYST_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while((SYST_CSR & BIT16)==0); // Bit 16 is the COUNTFLAG.  True when counter rolls over from zero.
	SYST_CSR = 0x00; // Disable Systick counter
}

void waitms(int len)
{
	while(len--) wait_1ms();
}

#define PIN_PERIOD (GPIO_B1) // Read period from PIO0_1 pin 12

// GetPeriod() seems to work fine for frequencies between 400Hz and 400kHz.
long int GetPeriod (int n)
{
	int i;
	unsigned int saved_TCNT1a, saved_TCNT1b;
	
	SYST_RVR = 0xffffff;  // 24-bit counter set to check for signal present
	SYST_CVR = 0xffffff; // load the SysTick counter
	SYST_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while (PIN_PERIOD!=0) // Wait for square wave to be 0
	{
		if(SYST_CSR & BIT16) return 0;
	}
	SYST_CSR = 0x00; // Disable Systick counter

	SYST_RVR = 0xffffff;  // 24-bit counter set to check for signal present
	SYST_CVR = 0xffffff; // load the SysTick counter
	SYST_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while (PIN_PERIOD==0) // Wait for square wave to be 1
	{
		if(SYST_CSR & BIT16) return 0;
	}
	SYST_CSR = 0x00; // Disable Systick counter
	
	SYST_RVR = 0xffffff;  // 24-bit counter reset
	SYST_CVR = 0xffffff; // load the SysTick counter to initial value
	SYST_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	for(i=0; i<n; i++) // Measure the time of 'n' periods
	{
		while (PIN_PERIOD!=0) // Wait for square wave to be 0
		{
			if(SYST_CSR & BIT16) return 0;
		}
		while (PIN_PERIOD==0) // Wait for square wave to be 1
		{
			if(SYST_CSR & BIT16) return 0;
		}
	}
	SYST_CSR = 0x00; // Disable Systick counter

	return 0xffffff-SYST_CVR;
}

void ConfigPins()
{
    GPIO_DIR0 &= ~(BIT1); // Configure PIO0_1 as input.
}

void main(void)
{
	long int count;
	float T, f;
	
	initUART(115200);
	enable_interrupts();
	ConfigPins();

	waitms(500); // Wait for putty to start.
	eputs("Period measurement using the Systick free running counter.\r\n"
	      "Connect signal to PIO0_1 (pin 12).\r\n");
	
	while(1)
	{
		count=GetPeriod(100);
		if(count>0)
		{
			T=count/(F_CPU*100.0);
			f=1/T;
			eputs("f=");
			PrintNumber(f, 10, 7);
			eputs("Hz, count=");
			PrintNumber(count, 10, 6);
			eputs("\r");
		}
		else
		{
			eputs("NO SIGNAL                     \r");
		}
		waitms(200);
	}
}
