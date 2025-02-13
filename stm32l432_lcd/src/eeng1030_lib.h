            #include <stdint.h>
#include <stm32l432xx.h>
void initClocks();
void enablePullUp(GPIO_TypeDef *Port, uint32_t BitNumber);
void pinMode(GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t Mode);
void selectAlternateFunction (GPIO_TypeDef *Port, uint32_t BitNumber, uint32_t AF);
void initSPI(SPI_TypeDef *spi);
uint8_t spi_exchange(SPI_TypeDef *spi,uint8_t d_out[], uint32_t d_out_len, uint8_t d_in[], uint32_t d_in_len);
void beginSPITransaction(SPI_TypeDef *spi);
void endSPITransaction(SPI_TypeDef *spi);