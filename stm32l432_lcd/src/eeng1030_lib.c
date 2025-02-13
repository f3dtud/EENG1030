#include <stdint.h>
#include <stm32l432xx.h>
void initClocks()
{
	// Initialize the clock system to a higher speed.
	// At boot time, the clock is derived from the MSI clock 
	// which defaults to 4MHz.  Will set it to 80MHz
	// See chapter 6 of the reference manual (RM0393)
	    RCC->CR &= ~(1 << 24); // Make sure PLL is off
	
	// PLL Input clock = MSI so BIT1 = 1, BIT 0 = 0
	// PLLM = Divisor for input clock : set = 1 so BIT6,5,4 = 0
	// PLL-VCO speed = PLL_N x PLL Input clock
	// This must be < 344MHz
	// PLL Input clock = 4MHz from MSI
	// PLL_N can range from 8 to 86.  
	// Will use 80 for PLL_N as 80 * 4 = 320MHz
	// Put value 80 into bits 14:8 (being sure to clear bits as necessary)
	// PLLSAI3 : Serial audio interface : not using leave BIT16 = 0
	// PLLP : Must pick a value that divides 320MHz down to <= 80MHz
	// If BIT17 = 1 then divisor is 17; 320/17 = 18.82MHz : ok (PLLP used by SAI)
	// PLLQEN : Don't need this so set BIT20 = 0
	// PLLQ : Must divide 320 down to value <=80MHz.  
	// Set BIT22,21 to 1 to get a divisor of 8 : ok
	// PLLREN : This enables the PLLCLK output of the PLL
	// I think we need this so set to 1. BIT24 = 1 
	// PLLR : Pick a value that divides 320 down to <= 80MHz
	// Choose 4 to give an 80MHz output.  
	// BIT26 = 0; BIT25 = 1
	// All other bits reserved and zero at reset
	    RCC->PLLCFGR = (1 << 25) + (1 << 24) + (1 << 22) + (1 << 21) + (1 << 17) + (80 << 8) + (1 << 0);	
	    RCC->CR |= (1 << 24); // Turn PLL on
	    while( (RCC->CR & (1 << 25))== 0); // Wait for PLL to be ready
	// configure flash for 4 wait states (required at 80MHz)
	    FLASH->ACR &= ~((1 << 2)+ (1 << 1) + (1 << 0));
	    FLASH->ACR |= (1 << 2); 
	    RCC->CFGR |= (1 << 1)+(1 << 0); // Select PLL as system clock
}
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber)
{
	Port->PUPDR = Port->PUPDR &~(3u << BitNumber*2); // clear pull-up resistor bits
	Port->PUPDR = Port->PUPDR | (1u << BitNumber*2); // set pull-up bit
}
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode)
{
	/*
        Modes : 00 = input
                01 = output
                10 = special function
                11 = analog mode
	*/
	uint32_t mode_value = Port->MODER;
	Mode = Mode << (2 * BitNumber);
	mode_value = mode_value & ~(3u << (BitNumber * 2));
	mode_value = mode_value | Mode;
	Port->MODER = mode_value;
}
void selectAlternateFunction (GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF)
{
    // The alternative function control is spread across two 32 bit registers AFR[0] and AFR[1]
    // There are 4 bits for each port bit.
    if (BitNumber < 8)
    {
        Port->AFR[0] &= ~(0x0f << (4*BitNumber));
        Port->AFR[0] |= (AF << (4*BitNumber));
    }
    else
    {
        BitNumber = BitNumber - 8;
        Port->AFR[1] &= ~(0x0f << (4*BitNumber));
        Port->AFR[1] |= (AF << (4*BitNumber));
    }
}
void initSPI(SPI_TypeDef *spi)
{
	/*	I/O List
		PA7  : SPI1 MOSI : Alternative function 5
		PB0  : SPI1 SSEL : Alternative function 5
		PA11 : SPI1 MISO : Alternative function 5
		PA1  : SPI1 SCLK : Alternative function 5    
*/
	int drain;	
	// Now configure the SPI interface
	drain = spi->SR;				// dummy read of SR to clear MODF	
	// enable SSM, set SSI, enable SPI, PCLK/2, MSB First Master, Clock = 1 when idle, CPOL=1 (SPI mode 3 overall)   
	spi->CR1 = (1 << 8)+(1 << 2) +(1 << 1) + (1 << 0) + (1<<3); // Assuming 4MHz default system clock set SPI speed to 1MHz (quite slow)
	spi->CR2 = (1 << 10)+(1 << 9)+(1 << 8)+(1 << 2); 	// configure for 8 bit operation

	pinMode(GPIOA,7,2);
    pinMode(GPIOB,0,2);
    pinMode(GPIOA,11,2);
    pinMode(GPIOA,1,2);
    selectAlternateFunction(GPIOA,7,5);
    selectAlternateFunction(GPIOB,0,5);
    selectAlternateFunction(GPIOA,11,5);
    selectAlternateFunction(GPIOA,1,5);
}

uint8_t spi_exchange(SPI_TypeDef *spi,uint8_t d_out[], uint32_t d_out_len, uint8_t d_in[], uint32_t d_in_len)
{   
    unsigned Timeout = 1000000;
    unsigned index=0;
    uint8_t ReturnValue=0;    
    uint8_t *preg=(uint8_t*)&spi->DR; // this is done to force 8 bit transfers and to suppress some warnings.
	beginSPITransaction(spi);
    while (((spi->SR & (1 << 7))!=0)&&(Timeout--)); // wait for any old transactions that may be pending to finish.
    while(d_out_len--) {
        *preg = d_out[index]; 
        index++;
        Timeout = 1000000;
        while (((spi->SR & (1 << 7))!=0)&&(Timeout--))
        {
            // wait for transfer to complete
        }        
        if (Timeout==0)
        {
            ReturnValue = -1;
        }
    }
    index=0;
    while(d_in_len--)
    {
        *preg=0xff;
        Timeout = 1000000;
        while (((spi->SR & (1 << 7))!=0)&&(Timeout--))
        {
            // wait for transfer to complete
        }
        if (Timeout==0)
        {
            ReturnValue = -1;
        }        
        d_in[index]=*preg;
    }
    endSPITransaction(spi);
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
