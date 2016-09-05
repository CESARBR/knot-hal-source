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
#define CONFIG			0x00
#define CONFIG_RST		0b00001000
#define CONFIG_MASK		0b01111111
#define CFG_MASK_RX_DR		0b01000000
#define CFG_MASK_TX_DS		0b00100000
#define CFG_MASK_MAX_RT		0b00010000
#define CFG_EN_CRC		0b00001000
#define CFG_CRCO		0b00000100
#define CFG_PWR_UP		0b00000010
#define CFG_PRIM_RX		0b00000001

/* RF status (reset value: 0b00001110) */
#define STATUS			0x07
#define STATUS_RST		0b00001110
#define STATUS_MASK		0x01110000
#define ST_RX_DR		0b01000000
#define ST_TX_DS		0b00100000
#define ST_MAX_RT		0b00010000
#define ST_RX_P_NO_MASK		0b00001110

/* read Data pipe number for the payload */
#define ST_RX_P_NO(v)	((v & ST_RX_P_NO_MASK) >> 1)

/* available in RX_FIFO: 0 <= number <= 5 valid values */
#define RX_FIFO_EMPTY		0b111
#define ST_TX_FULL		0b00000001
#define TX_FIFO_FULL		0b1


/*
 * Enable dynamic payload length (reset value: 0b00000000)
 * (requires EN_DLL in FEATURE and AA_Px in ENAA enabled)
 */
#define DYNPD			0x1c
#define DYNPD_RST		0b00000000
#define DYNPD_MASK		0b00111111
#define DPL_P5			0b00100000
#define DPL_P4			0b00010000
#define DPL_P3			0b00001000
#define DPL_P2			0b00000100
#define DPL_P1			0b00000010
#define DPL_P0			0b00000001

/* Enable dynamic payload length (reset value: 0b00000000) */
#define FEATURE			0x1d
#define FEATURE_RST		0b00000000
#define FEATURE_MASK		0b00000111
#define FT_EN_DPL		0b00000100
#define FT_EN_ACK_PAY		0b00000010
#define FT_EN_DYN_ACK		0b00000001


/* Enabled RX Address (reset value: 0b00000011) */
#define EN_RXADDR		0x02
#define EN_RXADDR_RST		0b00000011
#define EN_RXADDR_MASK		0b00111111
#define EN_RXADDR_P5		0b00100000
#define EN_RXADDR_P4		0b00010000
#define EN_RXADDR_P3		0b00001000
#define EN_RXADDR_P2		0b00000100
#define EN_RXADDR_P1		0b00000010
#define EN_RXADDR_P0		0b00000001

/* ---------------------------------------------
 * Command set for the nRF24L01(+) SPI
 */
/* 5 bit register address map */
#define REGISTER_MASK		0b00011111
#define R_REGISTER(r)		(0b00000000 | (r & REGISTER_MASK))
#define W_REGISTER(r)		(0b00100000 | (r & REGISTER_MASK))

#define R_RX_PAYLOAD		0b01100001

#define W_TX_PAYLOAD		0b10100000

#define FLUSH_TX		0b11100001
#define FLUSH_RX		0b11100010

#define REUSE_TX_PL		0b11100011

#define R_RX_PL_WID		0b01100000

/* 3 bit ack payload on pipe: 0 <= p <= 5 valid value */
#define W_ACK_PAYLOAD_MASK	0b00000111
#define W_ACK_PAYLOAD(p)	(0b10101000 | (p &  W_ACK_PAYLOAD_MASK))

#define W_TX_PAYLOAD_NOACK	0b10110000

#define NOP			0b11111111

/* always followed by data 0x73 */
#define ACTIVATE		0b01010000
#define ACTIVATE_DATA		0x73


/* Setup of Automatic Restransmission (reset value: 0b00000011) */
#define SETUP_RETR		0x04
#define SETUP_RETR_RST		0b00000011
#define RETR_ARD_MASK		0b11110000

/* write Auto Retransmit delay: 0 <= v <= 15 valid values */
#define RETR_ARD(v)		((v << 4) & RETR_ARD_MASK)
/* for delays respectively: 250us, 500us, 750us ... 4000us */
#define RETR_ARD_RD(v)		((v & RETR_ARD_MASK) >> 4) /* Read */
#define ARD_FACTOR_US		250
#define ARD_250US		0b0000
#define ARD_500US		0b0001
#define ARD_750US		0b0010
#define ARD_1000US		0b0011
#define ARD_1250US		0b0100
#define ARD_1500US		0b0101
#define ARD_40000US		0b1111
#define RETR_ARC_MASK		0b00001111

/* Auto Retransmit count: 0 <= v <= 15 valid values */
#define RETR_ARC(v)		(v & RETR_ARC_MASK)
#define ARC_DISABLE		0b0000

/* Setup of address widths (reset value: 0b00000011) */
#define SETUP_AW		0x03
#define SETUP_AW_RST		0b00000011
#define SETUP_AW_MASK		0b00000011
/* write: 0 <= b <= 5 valid values */
#define AW(b)			(b < 3 ? AW_INVALID : (b & SETUP_AW_MASK) - 2)
/* read: 0 <= b <= 5 valid values */
#define AW_RD(b) ((b & SETUP_AW_MASK) ? (b & SETUP_AW_MASK) + 2 : AW_INVALID)
#define AW_INVALID		0b00
#define AW_3BYTES		0b01
#define AW_4BYTES		0b10
#define AW_5BYTES		0b11

/* RF setup (reset value: 0b00001110) */
#define RF_SETUP		0x06
#define RF_SETUP_RST		0b00001110
#define RF_SETUP_MASK		0b10111110
#define RF_CONT_WAVE		0b10000000
#define RF_DR_LOW		0b00100000
#define RF_PLL_CLOCK		0b00010000
#define RF_DR_HIGH		0b00001000
#define RF_DR_MASK		(RF_DR_LOW | RF_DR_HIGH)
#define RF_DR(v)		(v & RF_DR_MASK)
#define DR_1MBPS		0b000000
#define DR_2MBPS		0b001000
#define DR_250KBPS		0b100000
#define RF_PWR_MASK		0b00000110

/*
 * write Power Amplifier(PA) control in
 * TX mode: 0 <= v <= 3 valid values
 */
#define RF_PWR(v)		((v << 1) & RF_PWR_MASK)

/*
 * and respectively to: -18dBm,
 * -12dBm, -6 dBm and 0dBm
 */
#define RF_PWR_RD(v)		((v & RF_PWR_MASK) >> 1)
#define PWR_18DBM		0b00
#define PWR_12DBM		0b01
#define PWR_6DBM		0b10
#define PWR_0DBM		0b11

/*
 * RF channel (reset value: 0b00000010),
 * frequencies from 2.400GHz to 2525GHz
 * 0 - 125 channels range for 1MHz
 * resolution bandwidth wider at 250Kbps/1Mbps
 */
#define RF_CH			0x05
/*
 * 0 - 62 channels range for 2MHz
 * resolution bandwidth wider at 2Mbps
 */
#define RF_CH_RST		0b00000010
#define RF_CH_MASK		0b01111111
#define CH(c)			(c & RF_CH_MASK)
#define CH_MIN			10
#define CH_MAX_2MBPS		54 /* 63 */
#define CH_MAX_1MBPS		116 /* 126 */

/*
 * Enable Auto Acknowledgment
 * (reset value: 0b00111111)
 */
#define EN_AA			0x01
#define EN_AA_RST		0b00111111
#define EN_AA_MASK		0b00111111
#define AA_P5			0b00100000
#define AA_P4			0b00010000
#define AA_P3			0b00001000
#define AA_P2			0b00000100
#define AA_P1			0b00000010
#define AA_P0			0b00000001

/* Receive address data pipe 0 (reset value: 0xe7e7e7e7e7) */
#define RX_ADDR_P0		0x0a
#define RX_ADDR_P0_RST		0xe7

/* Receive address data pipe 1 (reset value: 0xc2c2c2c2c2) */
#define RX_ADDR_P1		0x0b
#define RX_ADDR_P1_RST		0xc2

/* Receive address data pipe 2 (reset value: 0xc3) */
#define RX_ADDR_P2		0x0c
#define RX_ADDR_P2_RST		0xc3

/* Receive address data pipe 3 (reset value: 0xc4) */
#define RX_ADDR_P3		0x0d
#define RX_ADDR_P3_RST		0xc4

/* Receive address data pipe 4 (reset value: 0xc5) */
#define RX_ADDR_P4		0x0e
#define RX_ADDR_P4_RST		0xc5

/* Receive address data pipe 5 (reset value: 0xc6) */
#define RX_ADDR_P5		0x0f
#define RX_ADDR_P5_RST		0xc6

/* Transmit address (reset value: 0xe7e7e7e7e7) */
#define TX_ADDR			0x10
#define TX_ADDR_RST		0xe7

/* Number of  bytes in RX payload in data pipe 0 (reset value: 0b00000000) */
#define RX_PW_P0		0x11
#define RX_PW_P1		0x12
#define RX_PW_P2		0x13
#define RX_PW_P3		0x14
#define RX_PW_P4		0x15
#define RX_PW_P5		0x16


 /* FIFO status (reset value: 0b00010001) */
#define FIFO_STATUS		0x17
#define FIFO_STATUS_RST		0b00010001
#define FIFO_STATUS_MASK	0b01110011
#define FIFO_TX_REUSE		0b01000000
#define FIFO_TX_FULL		0b00100000
#define FIFO_TX_EMPTY		0b00010000
#define FIFO_RX_FULL		0b00000010
#define FIFO_RX_EMPTY		0b00000001

#ifdef __cplusplus
extern "C"{
#endif

/* IO functions*/
void delay_us(float us);
void delay_ms(float ms);
void enable(void);
void disable(void);
void io_setup(void);
void io_reset(void);


#ifdef __cplusplus
} // extern "C"
#endif
