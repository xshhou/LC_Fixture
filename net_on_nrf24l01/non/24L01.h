/*
 * 24L01.h
 *
 *  Created on: 2016Äê5ÔÂ4ÈÕ
 *      Author: WangJiuLing
 */

#ifndef DRIVER_24L01_H_
#define DRIVER_24L01_H_

#include "stm32f10x.h"

#define NRF24L01_CE_PIN		GPIO_Pin_4
#define NRF24L01_CE_GPIO	GPIOA

#define NRF24L01_CE_H	GPIO_SetBits(NRF24L01_CE_GPIO, NRF24L01_CE_PIN)
#define NRF24L01_CE_L	GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define NRF24L01_CSN_H	GPIO_SetBits(GPIOC, GPIO_Pin_4)
#define NRF24L01_CSN_L	GPIO_ResetBits(GPIOC, GPIO_Pin_4)
#define NRF24L01_IRQ	GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)

int nrf24l01_init(void);
int is_nrf24l01_exist(void);
void nrf24l01_tx_mode(void);
void nrf24l01_rx_mode(void);
u8 nrf24l01_tx_packet(u8 *txbuf);
u8 nrf24l01_rx_packet(u8 *rxbuf);
u8 nrf24l01_write_reg(u8 reg, u8 value);
u8 nrf24l01_read_reg(u8 reg);

#define ADDR_WIDTH	5
#define PLOAD_WIDTH	32

/* nRF24L01 interrupt flag */
#define MAX_RT  0x10	/* Max #of TX retrans interrupt */
#define TX_DS   0x20	/* TX data sent interrupt */
#define RX_DR   0x40	/* RX data received */
#define MAX_TX	MAX_RT
#define TX_OK	TX_DS
#define RX_OK	RX_DR
/* nRF24L01 commands */
#define NRF_READ_REG	0x00	/* Define read command to register */
#define NRF_WRITE_REG	0x20	/* Define write command to register */
#define RD_RX_PLOAD		0x61	/* Define RX payload register address */
#define WR_TX_PLOAD		0xA0	/* Define TX payload register address */
#define FLUSH_TX		0xE1	/* Define flush TX register command */
#define FLUSH_RX		0xE2	/* Define flush RX register command */
#define REUSE_TX_PL		0xE3	/* Define reuse TX payload register command */
#define NOP				0xFF	/* Define No Operation, might be used to read status register */
/* nRF24L01 registers addresses */
#define NOP				0xFF	/*  */
#define CONFIG			0x00	/* Configuration */
#define EN_AA			0x01	/* Enable Auto Acknowledgment */
#define EN_RXADDR		0x02	/* Enabled RX */
#define SETUP_AW		0x03	/* RF channel */
#define SETUP_RETR		0x04	/* Setup address width */
#define RF_CH			0x05	/* Setup Auto. Retrans */
#define RF_SETUP		0x06	/* RF setup */
#define STATUS			0x07	/* Status */
#define OBSERVE_TX		0x08	/* Observe TX */
#define CD				0x09	/* Carrier Detect */
#define RX_ADDR_P0		0x0A	/* RX address pipe0 */
#define RX_ADDR_P1		0x0B	/* RX address pipe1 */
#define RX_ADDR_P2		0x0C	/* RX address pipe2 */
#define RX_ADDR_P3		0x0D	/* RX address pipe3 */
#define RX_ADDR_P4		0x0E	/* RX address pipe4 */
#define RX_ADDR_P5		0x0F	/* RX address pipe5 */
#define TX_ADDR			0x10	/* TX address */
#define RX_PW_P0		0x11	/* RX payload width, pipe0 */
#define RX_PW_P1		0x12	/* RX payload width, pipe1 */
#define RX_PW_P2		0x13	/* RX payload width, pipe2 */
#define RX_PW_P3		0x14	/* RX payload width, pipe3 */
#define RX_PW_P4		0x15	/* RX payload width, pipe4 */
#define RX_PW_P5		0x16	/* RX payload width, pipe5 */
#define NRF_FIFO_STATUS	0x17	/* FIFO Status Register */

#endif /* DRIVER_24L01_H_ */
