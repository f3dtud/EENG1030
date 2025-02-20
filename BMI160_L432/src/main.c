#include "eeng1030_lib.h"
#include <stdio.h>
#include  <errno.h>
#include  <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
void setup(void);
void initI2C(void);
void delay(volatile uint32_t dly);
void initSerial(uint32_t baudrate);
void eputc(char c);

#define READ 1
#define WRITE 0

void ResetI2C();
void I2CStart(uint8_t address, int rw, int nbytes);
void I2CStop();
void I2CWrite(uint8_t Data);
uint8_t I2CRead();

// The following timeout value was determined by
// trial and error running the chip at 72MHz
// There is a little bit of 'headroom' included
#define I2C_TIMEOUT 20000
volatile uint16_t response;
volatile int16_t x_accel;
int main()
{
    char data[]={0x12,0x34,0x56};
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
		printf("Resp=%d\r\n",x_accel);
        delay(100000);

    }
}
void setup()
{
    RCC->AHB2ENR |= (1 << 0) | (1 << 1); // enable GPIOA and GPIOB
    initI2C();    
}
void initI2C(void)
{
    initClocks();
    pinMode(GPIOB,3,1); // make PB3 (internal LED) an output
    pinMode(GPIOB,6,2); // alternative functions for PB6 and PB7
    pinMode(GPIOB,7,2); 
    selectAlternateFunction(GPIOB,6,4); // Alternative function 4 = I2C1_SCL
    selectAlternateFunction(GPIOB,7,4); // Alternative function 4 = I2C1_SDA
    RCC->APB1ENR1 |= (1 << 21); // enable I2C1
    I2C1->CR1 = 0;
    delay(100);
	// 80MHz/(7+1)=10MHz.  10MHz/100kHz=100.  
	// So need (49+1) clock ticks for high part of clock + (49+1) for low part
	// Data setup delays are also added (Table 138 in the ref. manual was a bit of a help)
    I2C1->TIMINGR = (7 << 28) + (4 << 20) + (2 << 16) + (49 << 8) + (49); 
    I2C1->ICR |= 0xffff; // clear all pending interrupts
    I2C1->CR1 |= (1 << 0);
    

}
void delay(volatile uint32_t dly)
{
    while(dly--);
}
void ResetI2C()
{
	I2C1->CR1 &= ~(1 << 0);
	while(I2C1->CR1 & (1 << 0)); // forces a wait.
	I2C1->CR1 = (1 << 0);
}
void I2CStart(uint8_t address, int rw, int nbytes)
{
	unsigned timeout;
	unsigned Reg;
	Reg = I2C1->CR2;
	Reg &= ~(1 << 13); // clear START bit
	// Clear out old address.
	Reg &= ~(0x3ff); // clear out lower 10 bits of CR1 (address)
	Reg |= ((address<<1) & 0x3ff); // set address bits
	if (rw==READ)
		Reg |= (1 << 10); // read mode so set read bit
	else
		Reg &= ~(1 << 10); // write mode
	Reg &= ~(0x00ff0000); // clear byte count
	Reg |= (nbytes << 16); // set byte count
	//Reg |= (1 << 24); // set reload bit
	Reg |= (1 << 13); // set START bit
	I2C1->CR2 = Reg;
	timeout=I2C_TIMEOUT;
	while((I2C1->ISR & (1 << 0))==0); // wait for transmit complete
}
void I2CReStart(uint8_t address, int rw, int nbytes)
{
	unsigned timeout;
	unsigned Reg;
	Reg = I2C1->CR2;
	Reg &= ~(1<<13); // clear START bit
	// Clear out old address.
	Reg &= ~(0x3ff); // clear out lower 10 bits of CR1 (address)
	Reg |= ((address<<1) & 0x3ff); // set address bits
	if (rw==READ)
		Reg |= (1 << 10); // read mode so set read bit
	else
		Reg &= ~(1 << 10); // write mode
	Reg &= ~(0x00ff0000); // clear byte count
	Reg |= (nbytes << 16); // set byte count
	Reg &= ~(1 << 24); // clear reload bit
	Reg |= (1 << 13); // set START bit
	I2C1->CR2 = Reg;
	timeout=I2C_TIMEOUT;
	while((I2C1->ISR & (1 << 0))==0); // wait for transmit complete
}
void I2CStop()
{
	I2C1->CR2 &= ~(1 << 24); // clear reload bit
	delay(10);
	I2C1->CR2 |= (1 << 14);	// set stop bit
//	delay(100);
	while((I2C1->ISR & (1 << 0))==0); // wait for transmit complete

}

void I2CWrite(uint8_t Data)
{

	while((I2C1->ISR & (1 << 0))==0); // wait for transmit complete
	I2C1->TXDR = Data;
	//while((I2C1->ISR & (1 << 0))!=0); // wait for transmit to start
	//delay(2000);	
	while((I2C1->ISR & (1 << 0))==0); // wait for transmit complete
	
	
}
uint8_t I2CRead()
{
	unsigned timeout;
	//I2C1_CR2 |= BIT10; 		// read mode
	//I2C1_TXDR = 0xff;		// send a dummy byte
	//timeout = I2C_TIMEOUT;
	while((I2C1->ISR & (1 << 2))==0); // wait for receive complete
//	while((timeout--)&&!(I2C1->ISR & (1 << 2))); // wait for receive complete

	return I2C1->RXDR; 		// return rx data
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