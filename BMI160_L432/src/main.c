#include "eeng1030_lib.h"
#include <stdio.h>
#include  <errno.h>
#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include "i2c.h"
void setup(void);
void initI2C(void);
void delay(volatile uint32_t dly);
void initSerial(uint32_t baudrate);
void eputc(char c);

// The following timeout value was determined by
// trial and error running the chip at 72MHz
// There is a little bit of 'headroom' included
#define I2C_TIMEOUT 20000
uint16_t response;
int16_t x_accel;
int32_t X_g;
int main()
{
    setup();
	initSerial(9600);
    ResetI2C();
// Take accelerometer out of power down mode
	I2CStart(0x69,WRITE,2);
    I2CWrite(0x7e);  
	I2CWrite(0x11);  
	I2CStop();
	delay(1000000); // wait for power up.
    while(1)
    {	
		GPIOB->ODR |= (1 << 3);	// set port bit for logic analyser debug
        I2CStart(0x69,WRITE,1); // Write the address of the 
        I2CWrite(0x12);      	// register we want to talk to
		I2CReStart(0x69,READ,2);// Switch to read mode and request 2 bytes
		response=I2CRead();		// read low byte
		x_accel=response;  	
		response=I2CRead();		// read high byte
		x_accel=x_accel+(response << 8); // combine bytes
		I2CStop();	// end I2C transaction
		GPIOB->ODR &= ~(1 << 3); // clear port bit for logic analyser debug
		X_g = x_accel;	// promote to 32 bits and preserve sign
		X_g=(X_g*981)/16384; // assuming +1g ->16384 (+/-2g range)
		printf("X_g*100=%d\r\n",X_g);
        delay(100000);
    }
}
void setup()
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // enable GPIOA and GPIOB
	initClocks();
    initI2C();    
}

void delay(volatile uint32_t dly)
{
    while(dly--);
}

void initSerial(uint32_t baudrate)
{
    RCC->AHB2ENR |= (1 << 0); // make sure GPIOA is turned on
    pinMode(GPIOA,2,2); // alternate function mode for PA2
    selectAlternateFunction(GPIOA,2,7); // AF7 = USART2 TX
    pinMode(GPIOA,15,2); 
    selectAlternateFunction(GPIOA,15,3);
    RCC->APB1ENR1 |= (1 << 17); // turn on USART2

	const uint32_t CLOCK_SPEED=80000000;    
	uint32_t BaudRateDivisor;
	
	BaudRateDivisor = CLOCK_SPEED/baudrate;	
	USART2->CR1 = 0;
	USART2->CR2 = 0;
	USART2->CR3 = (1 << 12); // disable over-run errors
	USART2->BRR = BaudRateDivisor;
	USART2->CR1 =  (1 << 3);  // enable the transmitter and receiver
    USART2->CR1 |=  (1 << 2);  // enable the transmitter and receiver
	USART2->CR1 |= (1 << 0);
}
int _write(int file, char *data, int len)
{
    if ((file != STDOUT_FILENO) && (file != STDERR_FILENO))
    {
        errno = EBADF;
        return -1;
    }
    while(len--)
    {
        eputc(*data);    
        data++;
    }    
    return 0;
}
void eputc(char c)
{
    while( (USART2->ISR & (1 << 6))==0); // wait for ongoing transmission to finish
    USART2->TDR=c;
} 