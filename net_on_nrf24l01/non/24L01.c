/*
 * 24L01.c
 *
 *  Created on: 2016Äê5ÔÂ11ÈÕ
 *      Author: WangJiuLing
 */

#include "24L01.h"

u8 TX_ADDRESS[ADDR_WIDTH] = {0x34, 0x43, 0x10, 0x10, 0x01};
u8 RX_ADDRESS[ADDR_WIDTH] = {0x34, 0x43, 0x10, 0x10, 0x01};

static void nrf24L01_gpio_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA
						| RCC_APB2Periph_GPIOC
						| RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* set CS pin of SPI device high */
	GPIO_SetBits(GPIOA, GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource1);

	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void nrf24L01_spi_init()
{
	SPI_InitTypeDef SPI_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;// TODO
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI1, &SPI_InitStructure);

	SPI_Cmd(SPI1, ENABLE);
}

int nrf24l01_init(void)
{
	nrf24L01_gpio_init();
	nrf24L01_spi_init();

	NRF24L01_CE_L;
	NRF24L01_CSN_H;

	if(is_nrf24l01_exist() == 1){
		return -1;
	}

	return 0;
}

static u8 spi1_read_write_byte(u8 byte)
{
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	SPI_I2S_SendData(SPI1, byte);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

	return SPI_I2S_ReceiveData(SPI1);
}
u8 nrf24l01_write_reg(u8 reg, u8 value)
{
	u8 status;

	NRF24L01_CSN_L;
	status = spi1_read_write_byte(reg);
	spi1_read_write_byte(value);
	NRF24L01_CSN_H;

	return status;
}
u8 nrf24l01_read_reg(u8 reg)
{
	u8 value;

	NRF24L01_CSN_L;
	spi1_read_write_byte(reg);
	value = spi1_read_write_byte(0XFF);
	NRF24L01_CSN_H;

	return(value);
}
static u8 nrf24l01_write_buf(u8 reg, u8 *buf, u8 len)
{
	u8 status, i;

	NRF24L01_CSN_L;
	status = spi1_read_write_byte(reg);
	for(i = 0; i < len; i++)
		spi1_read_write_byte(*buf++);
	NRF24L01_CSN_H;

	return status;
}
static u8 nrf24l01_read_buf(u8 reg,u8 *buf,u8 len)
{
	u8 status, i;

	NRF24L01_CSN_L;
	status = spi1_read_write_byte(reg);
	for(i = 0; i < len; i++)
		buf[i] = spi1_read_write_byte(0);
	NRF24L01_CSN_H;

	return status;
}
int is_nrf24l01_exist(void)
{
	u8 buf[] = {0XA5};

	nrf24l01_write_buf(NRF_WRITE_REG + TX_ADDR, buf, 1);
	buf[0] = 0;
	nrf24l01_read_buf(TX_ADDR, buf, 1);
	if(buf[0] != 0XA5){
		return -1;
	}

	return 0;
}
/**
 * This function initializes one nRF24L01 device to TX mode,
 * set TX address, set RX address for auto.ack, fill TX payload,
 * select RF channel, datarate & TX pwr.
 * PWR_UP is set, CRC(2 bytes) is enabled, & PRIM:TX.
 *
 * ToDo: One high pulse(>10us) on CE will now send this
 *  packet and expext an acknowledgment from the RX device.
 */
void nrf24l01_tx_mode(void)
{
	NRF24L01_CE_L;
	/* Writes TX_Address to nRF24L01 */
	nrf24l01_write_buf(NRF_WRITE_REG + TX_ADDR, TX_ADDRESS, ADDR_WIDTH);
	/* Enable Pipe0 */
	nrf24l01_write_reg(NRF_WRITE_REG + EN_RXADDR, 0x01);
	/* Select RF channel 40 */
	nrf24l01_write_reg(NRF_WRITE_REG + RF_CH, 40);
	/* TX_PWR:0dBm, Datarate:2Mbps, LNA:HCURR */
	nrf24l01_write_reg(NRF_WRITE_REG + RF_SETUP, 0x0F);
	/* Set PWR_UP bit, enable CRC(2 bytes) & Prim:TX. MAX_RT & TX_DS enabled */
	nrf24l01_write_reg(NRF_WRITE_REG + CONFIG, 0x0e);
	NRF24L01_CE_H;
}
/**
 * This function initializes one nRF24L01 device to
 * RX Mode, set RX address, writes RX payload width,
 * select RF channel, datarate & LNA HCURR.
 * After init, CE is toggled high, which means that
 * this device is now ready to receive a datapacket.
 */
void nrf24l01_rx_mode(void)
{
	NRF24L01_CE_L;
	/* Use the same address on the RX device as the TX device */
	nrf24l01_write_buf(NRF_WRITE_REG + RX_ADDR_P0, TX_ADDRESS, ADDR_WIDTH);
	/* Enable Auto.Ack:Pipe0 */
//	nrf24l01_write_reg(NRF_WRITE_REG + EN_AA, 0x01);
	/* Enable Pipe0 */
	nrf24l01_write_reg(NRF_WRITE_REG + EN_RXADDR, 0x01);
	/* Select RF channel 40 */
	nrf24l01_write_reg(NRF_WRITE_REG + RF_CH, 40);
	/* Select same RX payload width as TX Payload width */
	nrf24l01_write_reg(NRF_WRITE_REG + RX_PW_P0, PLOAD_WIDTH);
	/* TX_PWR:0dBm, Datarate:2Mbps, LNA:HCURR */
	nrf24l01_write_reg(NRF_WRITE_REG + RF_SETUP, 0x0f);
	/* Set PWR_UP bit, enable CRC(2 bytes) & Prim:RX. RX_DR enabled */
	nrf24l01_write_reg(NRF_WRITE_REG + CONFIG, 0x0f);

	NRF24L01_CE_H; // Set CE pin high to enable RX device
}
u8 nrf24l01_tx_packet(u8 *buf)
{
	u8 sta;

	NRF24L01_CE_L;
	nrf24l01_write_buf(WR_TX_PLOAD, buf, PLOAD_WIDTH);
	NRF24L01_CE_H;
	while(NRF24L01_IRQ != 0);
	sta = spi1_read_write_byte(STATUS);
	nrf24l01_write_reg(NRF_WRITE_REG + STATUS, sta);
	nrf24l01_write_reg(FLUSH_TX, 0xff);

	return sta;
}
u8 nrf24l01_rx_packet(u8 *buf)
{
	u8 sta;

	sta = nrf24l01_read_reg(STATUS);
	nrf24l01_write_reg(NRF_WRITE_REG + STATUS, sta);
	if (sta & RX_OK) {
		nrf24l01_read_buf(RD_RX_PLOAD, buf, PLOAD_WIDTH);
		nrf24l01_write_reg(FLUSH_RX, 0xff);

		return RX_OK;
	}
	return 0;
}
