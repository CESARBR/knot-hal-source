/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
/* ---------------------------------------------
 * Register address map of nRF24L01(+)
*/
/* Configurator (reset value: 0b00001000) */
#define NRF24_CONFIG	0x00
#define NRF24_CONFIG_RST	0b00001000
#define NRF24_CONFIG_MASK	0b01111111
#define NRF24_CFG_MASK_RX_DR	0b01000000
#define NRF24_CFG_MASK_TX_DS	0b00100000
#define NRF24_CFG_MASK_MAX_RT	0b00010000
#define NRF24_CFG_EN_CRC	0b00001000
#define NRF24_CFG_CRCO	0b00000100
#define NRF24_CFG_PWR_UP	0b00000010
#define NRF24_CFG_PRIM_RX	0b00000001

/* RF status (reset value: 0b00001110) */
#define NRF24_STATUS	0x07
#define NRF24_STATUS_RST	0b00001110
#define NRF24_STATUS_MASK	0x01110000
#define NRF24_ST_RX_DR	0b01000000
#define NRF24_ST_TX_DS	0b00100000
#define NRF24_ST_MAX_RT	0b00010000
#define NRF24_ST_RX_P_NO_MASK	0b00001110
/* read Data pipe number for the payload */
#define NRF24_ST_RX_P_NO(v)	((v & NRF24_ST_RX_P_NO_MASK) >> 1)
/* available in RX_FIFO: 0 <= number <= 5 valid values */
#define NRF24_RX_FIFO_EMPTY		0b111
#define NRF24_ST_TX_FULL				0b00000001
#define NRF24_TX_FIFO_FULL			0b1
/* Read TX FIFO full flag */
#define ST_TX_STATUS(v)			((v) & NRF24_ST_TX_FULL)
/*
 * Enable dynamic payload length (reset value: 0b00000000)
 * (requires EN_DLL in FEATURE and AA_Px in ENAA enabled)
 */
#define NRF24_DYNPD				0x1c
#define NRF24_DYNPD_RST		0b00000000
#define NRF24_DYNPD_MASK		0b00111111
#define NRF24_DPL_P5			0b00100000
#define NRF24_DPL_P4			0b00010000
#define NRF24_DPL_P3			0b00001000
#define NRF24_DPL_P2			0b00000100
#define NRF24_DPL_P1			0b00000010
#define NRF24_DPL_P0			0b00000001

/* Enable dynamic payload length (reset value: 0b00000000) */
#define NRF24_FEATURE	0x1d
#define NRF24_FEATURE_RST	0b00000000
#define NRF24_FEATURE_MASK	0b00000111
#define NRF24_FT_EN_DPL	0b00000100
#define NRF24_FT_EN_ACK_PAY	0b00000010
#define NRF24_FT_EN_DYN_ACK	0b00000001


/* Enabled RX Address (reset value: 0b00000011) */
#define NRF24_EN_RXADDR	0x02
#define NRF24_EN_RXADDR_RST	0b00000011
#define NRF24_EN_RXADDR_MASK	0b00111111
#define NRF24_EN_RXADDR_P5	0b00100000
#define NRF24_EN_RXADDR_P4	0b00010000
#define NRF24_EN_RXADDR_P3	0b00001000
#define NRF24_EN_RXADDR_P2	0b00000100
#define NRF24_EN_RXADDR_P1	0b00000010
#define NRF24_EN_RXADDR_P0	0b00000001

/* ---------------------------------------------
 * Command set for the nRF24L01(+) SPI
*/
/* 5 bit register address map */
#define NRF24_REGISTER_MASK	0b00011111
#define NRF24_R_REGISTER(r)	(0b00000000 | (r & NRF24_REGISTER_MASK))
#define NRF24_W_REGISTER(r)	(0b00100000 | (r & NRF24_REGISTER_MASK))

#define NRF24_R_RX_PAYLOAD	0b01100001

#define NRF24_W_TX_PAYLOAD	0b10100000

#define NRF24_FLUSH_TX		0b11100001
#define NRF24_FLUSH_RX		0b11100010

#define NRF24_REUSE_TX_PL		0b11100011

#define NRF24_R_RX_PL_WID		0b01100000

/* 3 bit ack payload on pipe: 0 <= p <= 5 valid value */
#define NRF24_W_ACK_PAYLOAD_MASK	0b00000111
#define NRF24_W_ACK_PAYLOAD(p)	(0b10101000 | (p & NRF24_W_ACK_PAYLOAD_MASK))

#define NRF24_W_TX_PAYLOAD_NOACK	0b10110000

#define NRF24_NOP	0b11111111

/* always followed by data 0x73 */
#define NRF24_ACTIVATE	0b01010000
#define NRF24_ACTIVATE_DATA	0x73


/* Setup of Automatic Restransmission (reset value: 0b00000011) */
#define NRF24_SETUP_RETR	0x04
#define NRF24_SETUP_RETR_RST	0b00000011
#define NRF24_RETR_ARD_MASK		0b11110000
/* write Auto Retransmit delay: 0 <= v <= 15 valid values*/
#define NRF24_RETR_ARD(v)	((v << 4) & NRF24_RETR_ARD_MASK)
/* for delays respectively: 250us, 500us, 750us ... 1750us */
#define NRF24_RETR_ARD_RD(v)		((v & NRF24_RETR_ARD_MASK) >> 4)
#define NRF24_ARD_FACTOR_US		250
#define NRF24_ARD_250US					0b0000
#define NRF24_ARD_500US					0b0001
#define NRF24_ARD_750US					0b0010
#define NRF24_ARD_1000US				0b0011
#define NRF24_ARD_1250US				0b0100
#define NRF24_ARD_1500US				0b0101
#define NRF24_ARD_1750US				0b0110
#define NRF24_RETR_ARC_MASK		0b00001111
/* Auto Retransmit count: 0 <= v <= 15 valid values */
#define NRF24_RETR_ARC(v)	(v & NRF24_RETR_ARC_MASK)
#define NRF24_ARC_DISABLE				0b0000

/* Setup of address widths (reset value: 0b00000011) */
#define NRF24_SETUP_AW				0x03
#define NRF24_SETUP_AW_RST		0b00000011
#define NRF24_SETUP_AW_MASK	0b00000011
/* write: 0 <= b <= 5 valid values */
#define NRF24_AW(b)	(b < 3 ? NRF24_AW_INVALID :\
	(b & NRF24_SETUP_AW_MASK) - 2)
/* read: 0 <= b <= 5 valid values */
#define NRF24_AW_RD(b) (((b & NRF24_SETUP_AW_MASK) ?\
	(b & NRF24_SETUP_AW_MASK) + 2 : NRF24_AW_INVALID))
#define NRF24_AW_INVALID		0b00
#define NRF24_AW_3BYTES		0b01
#define NRF24_AW_4BYTES		0b10
#define NRF24_AW_5BYTES		0b11

/* RF setup (reset value: 0b00001110) */
#define NRF24_RF_SETUP				0x06
#define NRF24_RF_SETUP_RST		0b00001110
#define NRF24_RF_SETUP_MASK	0b10111110
#define NRF24_RF_CONT_WAVE	0b10000000
#define NRF24_RF_DR_LOW			0b00100000
#define NRF24_RF_PLL_CLOCK		0b00010000
#define NRF24_RF_DR_HIGH			0b00001000
#define NRF24_RF_DR_MASK	(NRF24_RF_DR_LOW | NRF24_RF_DR_HIGH)
#define NRF24_RF_DR(v)	(v & NRF24_RF_DR_MASK)
#define NRF24_DR_1MBPS				0b000000
#define NRF24_DR_2MBPS				0b001000
#define NRF24_DR_250KBPS			0b100000
#define NRF24_RF_PWR_MASK		0b00000110

/*
 * write Power Amplifier(PA) control in
 * TX mode: 0 <= v <= 3 valid values
 */

#define NRF24_RF_PWR(v)		((v << 1) & NRF24_RF_PWR_MASK)
/*
 * and respectively to: -18dBm,
 * -12dBm, -6 dBm and 0dBm
 */
#define NRF24_RF_PWR_RD(v)	((v & NRF24_RF_PWR_MASK) >> 1)
#define NRF24_PWR_18DBM		0b00
#define NRF24_PWR_12DBM		0b01
#define NRF24_PWR_6DBM		0b10
#define NRF24_PWR_0DBM		0b11

/*
 * RF channel (reset value: 0b00000010),
 * frequencies from 2.400GHz to 2525GHz
 * 0 - 125 channels range for 1MHz
 * resolution bandwidth wider at 250Kbps/1Mbps
 */
#define NRF24_RF_CH		0x05
/*
 * 0 - 62 channels range for 2MHz
 * resolution bandwidth wider at 2Mbps
 */
#define NRF24_RF_CH_RST	0b00000010
#define NRF24_RF_CH_MASK	0b01111111
#define NRF24_CH(c)		(c & NRF24_RF_CH_MASK)
#define NRF24_CH_MIN		10
#define NRF24_CH_MAX_2MBPS	54
#define NRF24_CH_MAX_1MBPS	116

/*
 * Enable Auto Acknowledgment
 * (reset value: 0b00111111)
 */
#define NRF24_EN_AA		0x01
#define NRF24_EN_AA_RST	0b00111111
#define NRF24_EN_AA_MASK	0b00111111
#define NRF24_AA_P5					0b00100000
#define NRF24_AA_P4					0b00010000
#define NRF24_AA_P3					0b00001000
#define NRF24_AA_P2					0b00000100
#define NRF24_AA_P1					0b00000010
#define NRF24_AA_P0					0b00000001

/* Receive address data pipe 0 (reset value: 0xe7e7e7e7e7) */
#define NRF24_RX_ADDR_P0				0x0a
#define NRF24_RX_ADDR_P0_RST	0xe7
/* Receive address data pipe 1 (reset value: 0xc2c2c2c2c2) */
#define NRF24_RX_ADDR_P1				0x0b
#define NRF24_RX_ADDR_P1_RST	0xc2
/* Receive address data pipe 2 (reset value: 0xc3) */
#define NRF24_RX_ADDR_P2				0x0c
#define NRF24_RX_ADDR_P2_RST	0xc3
/* Receive address data pipe 3 (reset value: 0xc4) */
#define NRF24_RX_ADDR_P3				0x0d
#define NRF24_RX_ADDR_P3_RST	0xc4
/* Receive address data pipe 4 (reset value: 0xc5) */
#define NRF24_RX_ADDR_P4				0x0e
#define NRF24_RX_ADDR_P4_RST	0xc5
/* Receive address data pipe 5 (reset value: 0xc6) */
#define NRF24_RX_ADDR_P5				0x0f
#define NRF24_RX_ADDR_P5_RST	0xc6
/* Transmit address (reset value: 0xe7e7e7e7e7) */
#define NRF24_TX_ADDR						0x10
#define NRF24_TX_ADDR_RST			0xe7

/* Number of  bytes in RX payload in data pipe 0 (reset value: 0b00000000) */
#define NRF24_RX_PW_P0			0x11
#define NRF24_RX_PW_P1			0x12
#define NRF24_RX_PW_P2			0x13
#define NRF24_RX_PW_P3			0x14
#define NRF24_RX_PW_P4			0x15
#define NRF24_RX_PW_P5			0x16


 /* FIFO status (reset value: 0b00010001) */
#define NRF24_FIFO_STATUS				0x17
#define NRF24_FIFO_STATUS_RST	0b00010001
#define NRF24_FIFO_STATUS_MASK	0b01110011
#define NRF24_FIFO_TX_REUSE			0b01000000
#define NRF24_FIFO_TX_FULL				0b00100000
#define NRF24_FIFO_TX_EMPTY			0b00010000
#define NRF24_FIFO_RX_FULL			0b00000010
#define NRF24_FIFO_RX_EMPTY			0b00000001

#ifdef __cplusplus
extern "C"{
#endif

/* IO functions*/
void delay_us(float us);
void enable(void);
void disable(void);
int io_setup(const char *dev);
void io_reset(int spi_fd);


#ifdef __cplusplus
} // extern "C"
#endif
