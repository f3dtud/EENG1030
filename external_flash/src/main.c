// using an MX25L005 external flash IC
/*

I/O List
PA7  : SPI1 MOSI : Alternative function 5
PB0  : SPI1 SSEL : Alternative function 5
PA11 : SPI1 MISO : Alternative function 5
PA1  : SPI1 SCLK : Alternative function 5    

*/
#include <stm32l432xx.h>
#include <stdint.h>
#include "eeng1030_lib.h"
#include <errno.h>
#include <sys/unistd.h> // STDOUT_FILENO, STDERR_FILENO
#include <stdio.h>
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void selectAlternateFunction (GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF);
void initSPI(SPI_TypeDef *spi);
uint8_t spi_exchange(SPI_TypeDef *spi,uint8_t d_out[], uint32_t d_out_len, uint8_t d_in[], uint32_t d_in_len);
void beginSPITransaction(SPI_TypeDef *spi);
void endSPITransaction(SPI_TypeDef *spi);
void delay(volatile uint32_t dly);

int read_electronic_signature(SPI_TypeDef *spi,uint8_t *sig,uint32_t sig_len);
int write_enable(SPI_TypeDef *spi);
int write_disable(SPI_TypeDef *spi);
int read_status_register(SPI_TypeDef *spi,uint8_t *status);
int write_status_register(SPI_TypeDef *spi,uint8_t status);
int power_down(SPI_TypeDef *spi);
int power_up(SPI_TypeDef *spi);
int read_data(SPI_TypeDef *spi,uint32_t address, uint8_t *data, uint32_t len);
int page_program(SPI_TypeDef *spi,uint32_t address, uint8_t *data, uint32_t len); // program page.  8LSB's of address should be 0.  len <= 256;
int sector_erase(SPI_TypeDef *spi,uint32_t sector_address);
int block_erase(SPI_TypeDef *spi,uint32_t block_address);
int chip_erase(SPI_TypeDef *spi);
int busy(SPI_TypeDef *spi);
void eputc(char c);
void initSerial(uint32_t baudrate);

void dumparray(uint8_t *ary, uint32_t len)
{
    while(len--)
    {
        printf("%x ",*ary);
        ary++;
    }
    printf("\r\n");
}
int main()
{
    setup();
    uint8_t temp;
    uint8_t dout[2];
    uint8_t din[10];
    uint8_t data[]={0xf1,0xe2,0xd3,0xc4,0xb5,0xa6};

    power_up(SPI1);
    write_enable(SPI1);
    //page_program(SPI1,0x0,data,6);    
    //chip_erase(SPI1);
    //sector_erase(SPI1,0);    
    while(1)
    {

        read_data(SPI1,0x0,din,10);        
        dumparray(din,10);        
    }
}
void delay(volatile uint32_t dly)
{
    while(dly--);
}
void setup()
{
    initClocks();
    initSerial(9600);
    RCC->AHB2ENR |=  (1 << 0) | (1 << 1); // turn on GPIOA and GPIOB
    RCC->APB2ENR |= (1 << 12); // turn on SPI 1
    // select alternative function mode for the SPI pins
    pinMode(GPIOA,7,2);
    pinMode(GPIOB,0,2);
    pinMode(GPIOA,11,2);
    pinMode(GPIOA,1,2);
    selectAlternateFunction(GPIOA,7,5);
    selectAlternateFunction(GPIOB,0,5);
    selectAlternateFunction(GPIOA,11,5);
    selectAlternateFunction(GPIOA,1,5);   
    initSPI(SPI1) ;
}
void initSPI(SPI_TypeDef *spi)
{
	int drain;	
	// Now configure the SPI interface
	drain = spi->SR;				// dummy read of SR to clear MODF	
	// enable SSM, set SSI, enable SPI, PCLK/2, MSB First Master, Clock = 1 when idle, CPOL=1 (SPI mode 3 overall)   
	spi->CR1 = (1<<9)+(1 << 8)+(1 << 2) +(1 << 1) + (1 << 0) + (5<<3); // Assuming 4MHz default system clock set SPI speed to 1MHz (quite slow)
	spi->CR2 = (1 << 10)+(1 << 9)+(1 << 8)+(1 << 2); 	// configure for 8 bit operation
}
uint8_t transferSPI8(SPI_TypeDef *spi,uint8_t data)
{
    uint8_t ReturnValue;
    volatile uint8_t *preg=(volatile uint8_t*)&spi->DR;	 // make sure no transfer is already under way
    while (((spi->SR & (1 << 7))!=0));
    *preg = data;
    while (((spi->SR & (1 << 7))!=0));// wait for transfer to finish
    ReturnValue = *preg;	
    return ReturnValue;
}
uint8_t spi_exchange(SPI_TypeDef *spi,uint8_t d_out[], uint32_t d_out_len, uint8_t d_in[], uint32_t d_in_len)
{   
    volatile uint32_t settling_time;
    unsigned index=0;
    uint8_t drain;
    uint8_t ReturnValue=0;    

    uint8_t *preg=(uint8_t*)&spi->DR; // this is done to force 8 bit transfers and to suppress some warnings.
	beginSPITransaction(spi);
    settling_time=20;
    while(settling_time--)
        __asm("nop");
    drain=spi->SR;

    while(d_out_len--) {    
        drain=transferSPI8(spi,d_out[index]);
        index++;        
    }
    
    index=0;
    while(d_in_len--)
    {        
        d_in[index]=transferSPI8(spi,0xff);                
        index++;
    }
    endSPITransaction(spi);
    settling_time=20;       // a little delay seems to be required here presumably to let CS signal rise 
    while(settling_time--)  // before being dropped again at a later stage
        __asm("nop");         
    return ReturnValue;
}
void beginSPITransaction(SPI_TypeDef *spi)
{
    spi->CR1 |= (1 << 6);
}
void endSPITransaction(SPI_TypeDef *spi)
{
	spi->CR1 &= ~(1 << 6);
}


int read_electronic_signature(SPI_TypeDef *spi,uint8_t *sig,uint32_t sig_len)
{
    uint8_t cmd=0x90;
	spi_exchange(spi,&cmd,1,sig,sig_len); // should return the values 0x13 and 0xc2 
	return 0;
}
int write_enable(SPI_TypeDef *spi)
{
    uint8_t cmd=0x06;
    uint8_t rxdata[1];
	spi_exchange(spi,&cmd,1,rxdata,0); 
	return 0;
}
int write_disable(SPI_TypeDef *spi)
{
    uint8_t cmd=0x04;
    uint8_t rxdata[1];
	spi_exchange(spi,&cmd,1,rxdata,0); 
	return 0;  
}
int read_status_register(SPI_TypeDef *spi,uint8_t *status)
{    
	uint8_t cmd=0x05;
	uint8_t rxdata[2];
	int ret;
	spi_exchange(spi,&cmd,1,rxdata,1); 
	*status=rxdata[0];
    delay(200);
	return 0;
}
int write_status_register(SPI_TypeDef *spi,uint8_t status)
{
    uint8_t txdata[2];
	txdata[0]=0x04;
	txdata[1]=status;	
    uint8_t rxdata[1];
	spi_exchange(spi,txdata,2,rxdata,0);
	return 0;
}
int power_down(SPI_TypeDef *spi)
{
    uint8_t cmd=0xb9;
    uint8_t rxdata[1];
	spi_exchange(spi,&cmd,1,rxdata,0); 
	return 0;
}
int power_up(SPI_TypeDef *spi)
{
    uint8_t cmd=0xab;
    uint8_t rxdata[1];
	spi_exchange(spi,&cmd,1,rxdata,0); 
	return 0;
}
int read_data(SPI_TypeDef *spi,uint32_t address, uint8_t *data, uint32_t len)
{
	uint8_t txdata[4];
	txdata[0]=0x03;
	txdata[1]=address>>16;	
	txdata[2]=(address >> 8) & 0xff;
	txdata[3]=address & 0xff;	
    spi_exchange(spi,txdata,4,data,len);	
	return 0;
}

int page_program(SPI_TypeDef *spi,uint32_t address, uint8_t *data, uint32_t len) // program page.  8LSB's of address should be 0.  len <= 256;
{
    uint8_t txdata[4+len];
    uint8_t rxdata[1];
    volatile uint32_t busycount=0;
	txdata[0]=0x02;
	txdata[1]=address>>16;	
	txdata[2]=(address >> 8) & 0xff;
	txdata[3]=address & 0xff;
    for (uint32_t i=0;i<len;i++)
    {
        txdata[4+i]=data[i];
    }
    spi_exchange(spi,txdata,4+len,rxdata,0);	
	while (busy(spi))
	{
		busycount++;
	}
	return 0;
}
int sector_erase(SPI_TypeDef *spi,uint32_t sector_address)
{
    uint8_t txdata[5];
    uint8_t rxdata[1];
    volatile uint32_t busycount=0;

	txdata[0]=0x20;
	txdata[1]=sector_address>>16;	
	txdata[2]=(sector_address >> 8) & 0xff;
	txdata[3]=0;
    spi_exchange(spi,txdata,4,rxdata,0);		
	while (busy(spi))
	{
		busycount++;
	}
	return 0;
}
int block_erase(SPI_TypeDef *spi,uint32_t block_address)
{
    uint8_t txdata[5];
    uint8_t rxdata[1];
    volatile uint32_t busycount=0;

    txdata[0]=0x52;
	txdata[1]=block_address>>16;	
	txdata[2]=(block_address >> 8) & 0xff;
	txdata[3]=0;
    spi_exchange(spi,txdata,4,rxdata,0);		
	while (busy(spi))
	{
		busycount++;
	}
	return 0;
}
int chip_erase(SPI_TypeDef *spi)
{
    uint8_t txdata[5];
    uint8_t rxdata[1];
    volatile uint32_t busycount=0;
    txdata[0]=0x60;	
    spi_exchange(spi,txdata,1,rxdata,0);		
	while (busy(spi))
	{
		busycount++;
	}
	return 0;
}
int busy(SPI_TypeDef *spi)
{
// Must now wait for write to complete.
// Poll the Write In Progess bit in the status register (LSB)
	uint8_t stat;
	read_status_register(spi,&stat);
	return (stat & 1);
}

void initSerial(uint32_t baudrate)
{
    RCC->AHB2ENR |= (1 << 0); // make sure GPIOA is turned on
    pinMode(GPIOA,2,2); // alternate function mode for PA2
    selectAlternateFunction(GPIOA,2,7); // AF7 = USART2 TX

    RCC->APB1ENR1 |= (1 << 17); // turn on USART2

	const uint32_t CLOCK_SPEED=80000000;    
	uint32_t BaudRateDivisor;
	
	BaudRateDivisor = CLOCK_SPEED/baudrate;	
	USART2->CR1 = 0;
	USART2->CR2 = 0;
	USART2->CR3 = (1 << 12); // disable over-run errors
	USART2->BRR = BaudRateDivisor;
	USART2->CR1 =  (1 << 3);  // enable the transmitter
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
