/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */


#define CFG_sx1272_radio			1
//#define CFG_sx1276_radio			1

#define CFG_us915				1
//#define CFG_eu868				1

#define DISABLE_INVERT_IQ_ON_RX			1

#define US_PER_OSTICK				50

#define UNUSED_PIN				255

// ----------------------------------------
// Registers Mapping
#define RegFifo					0x00 // common
#define RegOpMode				0x01 // common
#define FSKRegBitrateMsb			0x02
#define FSKRegBitrateLsb			0x03
#define FSKRegFdevMsb				0x04
#define FSKRegFdevLsb				0x05
#define RegFrfMsb				0x06 // common
#define RegFrfMid				0x07 // common
#define RegFrfLsb				0x08 // common
#define RegPaConfig				0x09 // common
#define RegPaRamp				0x0A // common
#define RegOcp					0x0B // common
#define RegLna					0x0C // common
#define FSKRegRxConfig				0x0D
#define LORARegFifoAddrPtr			0x0D
#define FSKRegRssiConfig			0x0E
#define LORARegFifoTxBaseAddr			0x0E
#define FSKRegRssiCollision			0x0F
#define LORARegFifoRxBaseAddr			0x0F
#define FSKRegRssiThresh			0x10
#define LORARegFifoRxCurrentAddr		0x10
#define FSKRegRssiValue				0x11
#define LORARegIrqFlagsMask			0x11
#define FSKRegRxBw				0x12
#define LORARegIrqFlags				0x12
#define FSKRegAfcBw				0x13
#define LORARegRxNbBytes			0x13
#define FSKRegOokPeak				0x14
#define LORARegRxHeaderCntValueMsb		0x14
#define FSKRegOokFix				0x15
#define LORARegRxHeaderCntValueLsb		0x15
#define FSKRegOokAvg				0x16
#define LORARegRxPacketCntValueMsb		0x16
#define LORARegRxpacketCntValueLsb		0x17
#define LORARegModemStat			0x18
#define LORARegPktSnrValue			0x19
#define FSKRegAfcFei				0x1A
#define LORARegPktRssiValue			0x1A
#define FSKRegAfcMsb				0x1B
#define LORARegRssiValue			0x1B
#define FSKRegAfcLsb				0x1C
#define LORARegHopChannel			0x1C
#define FSKRegFeiMsb				0x1D
#define LORARegModemConfig1			0x1D
#define FSKRegFeiLsb				0x1E
#define LORARegModemConfig2			0x1E
#define FSKRegPreambleDetect			0x1F
#define LORARegSymbTimeoutLsb			0x1F
#define FSKRegRxTimeout1			0x20
#define LORARegPreambleMsb			0x20
#define FSKRegRxTimeout2			0x21
#define LORARegPreambleLsb			0x21
#define FSKRegRxTimeout3			0x22
#define LORARegPayloadLength			0x22
#define FSKRegRxDelay				0x23
#define LORARegPayloadMaxLength			0x23
#define FSKRegOsc				0x24
#define LORARegHopPeriod			0x24
#define FSKRegPreambleMsb			0x25
#define LORARegFifoRxByteAddr			0x25
#define LORARegModemConfig3			0x26
#define FSKRegPreambleLsb			0x26
#define FSKRegSyncConfig			0x27
#define LORARegFeiMsb				0x28
#define FSKRegSyncValue1			0x28
#define LORAFeiMib				0x29
#define FSKRegSyncValue2			0x29
#define LORARegFeiLsb				0x2A
#define FSKRegSyncValue3			0x2A
#define FSKRegSyncValue4			0x2B
#define LORARegRssiWideband			0x2C
#define FSKRegSyncValue5			0x2C
#define FSKRegSyncValue6			0x2D
#define FSKRegSyncValue7			0x2E
#define FSKRegSyncValue8			0x2F
#define FSKRegPacketConfig1			0x30
#define FSKRegPacketConfig2			0x31
#define LORARegDetectOptimize			0x31
#define FSKRegPayloadLength			0x32
#define FSKRegNodeAdrs				0x33
#define LORARegInvertIQ				0x33
#define FSKRegBroadcastAdrs			0x34
#define FSKRegFifoThresh			0x35
#define FSKRegSeqConfig1			0x36
#define FSKRegSeqConfig2			0x37
#define LORARegDetectionThreshold		0x37
#define FSKRegTimerResol			0x38
#define FSKRegTimer1Coef			0x39
#define LORARegSyncWord				0x39
#define FSKRegTimer2Coef			0x3A
#define FSKRegImageCal				0x3B
#define FSKRegTemp				0x3C
#define FSKRegLowBat				0x3D
#define FSKRegIrqFlags1				0x3E
#define FSKRegIrqFlags2				0x3F
#define RegDioMapping1				0x40 // common
#define RegDioMapping2				0x41 // common
#define RegVersion				0x42 // common
// #define RegAgcRef                                  0x43 // common
// #define RegAgcThresh1                              0x44 // common
// #define RegAgcThresh2                              0x45 // common
// #define RegAgcThresh3                              0x46 // common
// #define RegPllHop                                  0x4B // common
// #define RegTcxo                                    0x58 // common
#define RegPaDac				0x5A // common
// #define RegPll                                     0x5C // common
// #define RegPllLowPn                                0x5E // common
// #define RegFormerTemp                              0x6C // common
// #define RegBitRateFrac                             0x70 // common

// ----------------------------------------
// spread factors and mode for RegModemConfig2
#define SX1272_MC2_FSK				0x00
#define SX1272_MC2_SF7				0x70
#define SX1272_MC2_SF8				0x80
#define SX1272_MC2_SF9				0x90
#define SX1272_MC2_SF10				0xA0
#define SX1272_MC2_SF11				0xB0
#define SX1272_MC2_SF12				0xC0
// bandwidth for RegModemConfig1
#define SX1272_MC1_BW_125			0x00
#define SX1272_MC1_BW_250			0x40
#define SX1272_MC1_BW_500			0x80
// coding rate for RegModemConfig1
#define SX1272_MC1_CR_4_5			0x08
#define SX1272_MC1_CR_4_6			0x10
#define SX1272_MC1_CR_4_7			0x18
#define SX1272_MC1_CR_4_8			0x20
#define SX1272_MC1_IMPLICIT_HEADER_MODE_ON	0x04 // required for receive
#define SX1272_MC1_RX_PAYLOAD_CRCON		0x02
// mandated for SF11 and SF12
#define SX1272_MC1_LOW_DATA_RATE_OPTIMIZE	0x01
// transmit power configuration for RegPaConfig
#define SX1272_PAC_PA_SELECT_PA_BOOST		0x80
#define SX1272_PAC_PA_SELECT_RFIO_PIN		0x00


// sx1276 RegModemConfig1
#define SX1276_MC1_BW_125			0x70
#define SX1276_MC1_BW_250			0x80
#define SX1276_MC1_BW_500			0x90
#define SX1276_MC1_CR_4_5			0x02
#define SX1276_MC1_CR_4_6			0x04
#define SX1276_MC1_CR_4_7			0x06
#define SX1276_MC1_CR_4_8			0x08

#define SX1276_MC1_IMPLICIT_HEADER_MODE_ON	0x01

// sx1276 RegModemConfig2
#define SX1276_MC2_RX_PAYLOAD_CRCON		0x04

// sx1276 RegModemConfig3
#define SX1276_MC3_LOW_DATA_RATE_OPTIMIZE	0x08
#define SX1276_MC3_AGCAUTO			0x04

// preamble for lora networks (nibbles swapped)
#define LORA_MAC_PREAMBLE			0x34

#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1	0x0A
#ifdef CFG_sx1276_radio
#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2	0x70
#elif CFG_sx1272_radio
#define RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2	0x74
#endif

// ----------------------------------------
// Constants for radio registers
#define OPMODE_LORA				0x80
#define OPMODE_MASK				0x07
#define OPMODE_SLEEP				0x00
#define OPMODE_STANDBY				0x01
#define OPMODE_FSTX				0x02
#define OPMODE_TX				0x03
#define OPMODE_FSRX				0x04
#define OPMODE_RX				0x05
#define OPMODE_RX_SINGLE			0x06
#define OPMODE_CAD				0x07

// ----------------------------------------
// Bits masking the corresponding IRQs from the radio
#define IRQ_LORA_RXTOUT_MASK			0x80
#define IRQ_LORA_RXDONE_MASK			0x40
#define IRQ_LORA_CRCERR_MASK			0x20
#define IRQ_LORA_HEADER_MASK			0x10
#define IRQ_LORA_TXDONE_MASK			0x08
#define IRQ_LORA_CDDONE_MASK			0x04
#define IRQ_LORA_FHSSCH_MASK			0x02
#define IRQ_LORA_CDDETD_MASK			0x01

#define IRQ_FSK1_MODEREADY_MASK			0x80
#define IRQ_FSK1_RXREADY_MASK			0x40
#define IRQ_FSK1_TXREADY_MASK			0x20
#define IRQ_FSK1_PLLLOCK_MASK			0x10
#define IRQ_FSK1_RSSI_MASK			0x08
#define IRQ_FSK1_TIMEOUT_MASK			0x04
#define IRQ_FSK1_PREAMBLEDETECT_MASK		0x02
#define IRQ_FSK1_SYNCADDRESSMATCH_MASK		0x01
#define IRQ_FSK2_FIFOFULL_MASK			0x80
#define IRQ_FSK2_FIFOEMPTY_MASK			0x40
#define IRQ_FSK2_FIFOLEVEL_MASK			0x20
#define IRQ_FSK2_FIFOOVERRUN_MASK		0x10
#define IRQ_FSK2_PACKETSENT_MASK		0x08
#define IRQ_FSK2_PAYLOADREADY_MASK		0x04
#define IRQ_FSK2_CRCOK_MASK			0x02
#define IRQ_FSK2_LOWBAT_MASK			0x01

// ----------------------------------------
// DIO function mappings				D0D1D2D3
#define MAP_DIO0_LORA_RXDONE			0x00  // 00------
#define MAP_DIO0_LORA_TXDONE			0x40  // 01------
#define MAP_DIO1_LORA_RXTOUT			0x00  // --00----
#define MAP_DIO1_LORA_NOP			0x30  // --11----
#define MAP_DIO2_LORA_NOP			0xC0  // ----11--

	//(packet sent / payload ready)
#define MAP_DIO0_FSK_READY			0x00  // 00------
#define MAP_DIO1_FSK_NOP			0x30  // --11----
#define MAP_DIO2_FSK_TXNOP			0x04  // ----01--
#define MAP_DIO2_FSK_TIMEOUT			0x08  // ----10--


// FSK IMAGECAL defines
#define RF_IMAGECAL_AUTOIMAGECAL_MASK		0x7F
#define RF_IMAGECAL_AUTOIMAGECAL_ON		0x80
#define RF_IMAGECAL_AUTOIMAGECAL_OFF		0x00  // Default

#define RF_IMAGECAL_IMAGECAL_MASK		0xBF
#define RF_IMAGECAL_IMAGECAL_START		0x40

#define RF_IMAGECAL_IMAGECAL_RUNNING		0x20
#define RF_IMAGECAL_IMAGECAL_DONE		0x00  // Default


#ifdef CFG_sx1276_radio
#define LNA_RX_GAIN (0x20|0x1)
#elif CFG_sx1272_radio
#define LNA_RX_GAIN (0x20|0x03)
#else
//#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif

#include <stdint.h>

//TYPEDEFS----------------------------------------------------------------------
typedef int32_t  ostime_t;
enum _cr_t { CR_4_5=0, CR_4_6, CR_4_7, CR_4_8 };
enum _sf_t { FSK=0, SF7, SF8, SF9, SF10, SF11, SF12, SFrfu };
enum _bw_t { BW125=0, BW250, BW500, BWrfu };
typedef uint8_t cr_t;
typedef uint8_t sf_t;
typedef uint8_t bw_t;
typedef uint8_t dr_t;

#define DEFINE_LMIC  struct lmic_t LMIC
#define DECLARE_LMIC extern struct lmic_t LMIC


#ifndef RX_RAMPUP
#define RX_RAMPUP  (us2osticks(2000))
#endif
#ifndef TX_RAMPUP
#define TX_RAMPUP  (us2osticks(2000))
#endif

#ifndef OSTICKS_PER_SEC
#define OSTICKS_PER_SEC 32768
#elif OSTICKS_PER_SEC < 10000 || OSTICKS_PER_SEC > 64516
#error Illegal OSTICKS_PER_SEC - must be in range \
			[10000:64516]. One tick must be 15.5us .. 100us long.
#endif

#if !HAS_ostick_conv
#define us2osticks(us)	((ostime_t) \
				( ((int64_t)(us) * OSTICKS_PER_SEC) / 1000000))
#define ms2osticks(ms)	((ostime_t) \
				( ((int64_t)(ms) * OSTICKS_PER_SEC)    / 1000))
#define sec2osticks(sec) ((ostime_t) \
				( (int64_t)(sec) * OSTICKS_PER_SEC))
#define osticks2ms(os)	((int32_t)(((os)*(int64_t)1000    ) / OSTICKS_PER_SEC))
#define osticks2us(os)	((int32_t)(((os)*(int64_t)1000000 ) / OSTICKS_PER_SEC))
// Special versions
#define us2osticksCeil(us) ((ostime_t) \
			( ((int64_t)(us) * OSTICKS_PER_SEC + 999999) / 1000000))
#define us2osticksRound(us) ((ostime_t) \
			( ((int64_t)(us) * OSTICKS_PER_SEC + 500000) / 1000000))
#define ms2osticksCeil(ms) ((ostime_t) \
			( ((int64_t)(ms) * OSTICKS_PER_SEC + 999) / 1000))
#define ms2osticksRound(ms) ((ostime_t) \
			( ((int64_t)(ms) * OSTICKS_PER_SEC + 500) / 1000))
#endif


enum { ILLEGAL_RPS = 0xFF };
enum { DR_PAGE_EU868 = 0x00 };
enum { DR_PAGE_US915 = 0x10 };

// Global maximum frame length
enum { STD_PREAMBLE_LEN	=  8 };
enum { MAX_LEN_FRAME	= 64 };
enum { LEN_DEVNONCE	=  2 };
enum { LEN_ARTNONCE	=  3 };
enum { LEN_NETID	=  3 };
enum { DELAY_JACC1	=  5 }; // in secs
enum { DELAY_DNW1	=  1 }; // in secs down window #1
enum { DELAY_EXTDNW2	=  1 }; // in secs
enum { DELAY_JACC2	=  DELAY_JACC1+(int)DELAY_EXTDNW2 }; // in secs
enum { DELAY_DNW2	=  DELAY_DNW1 +(int)DELAY_EXTDNW2 }; // in secs
enum { BCN_INTV_exp	= 7 };
enum { BCN_INTV_sec	= 1<<BCN_INTV_exp };
enum { BCN_INTV_ms	= BCN_INTV_sec*1000L };
enum { BCN_INTV_us	= BCN_INTV_ms*1000L };
// space reserved for beacon and NWK management
enum { BCN_RESERVE_ms	= 2120 };
// end of beacon period to prevent interference with beacon
enum { BCN_GUARD_ms	= 3000 };
// 2^12 reception slots a this span
enum { BCN_SLOT_SPAN_ms	=   30 };
enum { BCN_WINDOW_ms	= BCN_INTV_ms-(int)BCN_GUARD_ms-(int)BCN_RESERVE_ms };
enum { BCN_RESERVE_us	= 2120000 };
enum { BCN_GUARD_us	= 3000000 };
enum { BCN_SLOT_SPAN_us	=   30000 };

#if defined(CFG_eu868) // ==============================================

enum _dr_eu868_t { DR_SF12=0, DR_SF11, DR_SF10, DR_SF9, DR_SF8, DR_SF7, DR_SF7B,
							DR_FSK, DR_NONE };
enum { DR_DFLTMIN = DR_SF7 };
enum { DR_PAGE = DR_PAGE_EU868 };

// Default frequency plan for EU 868MHz ISM band
// Bands:
//  g1 :   1%  14dBm
//  g2 : 0.1%  14dBm
//  g3 :  10%  27dBm
//                 freq             band     datarates
enum { EU868_F1 = 868100000,      // g1   SF7-12
	EU868_F2 = 868300000,      // g1   SF7-12 FSK SF7/250
	EU868_F3 = 868500000,      // g1   SF7-12
	EU868_F4 = 868850000,      // g2   SF7-12
	EU868_F5 = 869050000,      // g2   SF7-12
	EU868_F6 = 869525000,      // g3   SF7-12
	EU868_J4 = 864100000,      // g2   SF7-12  used during join
	EU868_J5 = 864300000,      // g2   SF7-12   ditto
	EU868_J6 = 864500000,      // g2   SF7-12   ditto
};
enum { EU868_FREQ_MIN = 863000000,
       EU868_FREQ_MAX = 870000000 };

enum { CHNL_PING	= 5 };
enum { FREQ_PING	= EU868_F6 };  // default ping freq
enum { DR_PING		= DR_SF9 };    // default ping DR
enum { CHNL_DNW2	= 5 };
enum { FREQ_DNW2	= EU868_F6 };
enum { DR_DNW2		= DR_SF12 };
enum { CHNL_BCN		= 5 };
enum { FREQ_BCN		= EU868_F6 };
enum { DR_BCN		= DR_SF9 };
enum { AIRTIME_BCN	= 144384 };  // micros

#elif defined(CFG_us915)  // =========================================

enum { MAX_XCHANNELS = 2 };// extra channels in RAM, channels 0-71 are immutable
enum { MAX_TXPOW_125kHz = 30 };

enum { RADIO_RST=0, RADIO_TX=1, RADIO_RX=2, RADIO_RXON=3 };

enum { RXMODE_SINGLE, RXMODE_SCAN, RXMODE_RSSI };

enum _dr_us915_t { DR_SF10=0, DR_SF9, DR_SF8, DR_SF7, DR_SF8C, DR_NONE,
	// Devices behind a router:
	DR_SF12CR=8, DR_SF11CR, DR_SF10CR, DR_SF9CR, DR_SF8CR, DR_SF7CR };
enum { DR_DFLTMIN = DR_SF8C };
enum { DR_PAGE = DR_PAGE_US915 };

// Default frequency plan for US 915MHz
enum { US915_125kHz_UPFBASE = 902300000,
	US915_125kHz_UPFSTEP =    200000,
	US915_500kHz_UPFBASE = 903000000,
	US915_500kHz_UPFSTEP =   1600000,
	US915_500kHz_DNFBASE = 923300000,
	US915_500kHz_DNFSTEP =    600000
};

enum { US915_FREQ_MIN = 902000000, US915_FREQ_MAX = 928000000 };
// used only for default init of state (follows beacon - rotating)
enum { CHNL_PING	= 0 };
// default ping freq
enum { FREQ_PING	= US915_500kHz_DNFBASE +
					CHNL_PING*US915_500kHz_DNFSTEP };
enum { DR_PING		= DR_SF10CR };	// default ping DR
enum { CHNL_DNW2	= 0 };
enum { FREQ_DNW2	= US915_500kHz_DNFBASE +
					CHNL_DNW2*US915_500kHz_DNFSTEP };
enum { DR_DNW2		= DR_SF12CR };
// used only for default init of state (rotating beacon scheme)
enum { CHNL_BCN		= 0 };
enum { DR_BCN		= DR_SF10CR };
enum { AIRTIME_BCN	= 72192 };	// micros

#endif // ======================================================================

enum {
	// Join Request frame format
	OFF_JR_HDR	= 0,
	OFF_JR_ARTEUI	= 1,
	OFF_JR_DEVEUI	= 9,
	OFF_JR_DEVNONCE	= 17,
	OFF_JR_MIC	= 19,
	LEN_JR		= 23
};
enum {
	// Join Accept frame format
	OFF_JA_HDR	= 0,
	OFF_JA_ARTNONCE	= 1,
	OFF_JA_NETID	= 4,
	OFF_JA_DEVADDR	= 7,
	OFF_JA_RFU	= 11,
	OFF_JA_DLSET	= 11,
	OFF_JA_RXDLY	= 12,
	OFF_CFLIST	= 13,
	LEN_JA		= 17,
	LEN_JAEXT	= 17+16
};
enum {
	// Data frame format
	OFF_DAT_HDR	= 0,
	OFF_DAT_ADDR	= 1,
	OFF_DAT_FCT	= 5,
	OFF_DAT_SEQNO	= 6,
	OFF_DAT_OPTS	= 8,
};
enum { MAX_LEN_PAYLOAD = MAX_LEN_FRAME-(int)OFF_DAT_OPTS-4 };
enum {
	// Bitfields in frame format octet
	HDR_FTYPE	= 0xE0,
	HDR_RFU		= 0x1C,
	HDR_MAJOR	= 0x03
};
enum { HDR_FTYPE_DNFLAG = 0x20 };  // flags DN frame except for HDR_FTYPE_PROP
enum {
	// Values of frame type bit field
	HDR_FTYPE_JREQ		= 0x00,
	HDR_FTYPE_JACC		= 0x20,
	HDR_FTYPE_DAUP		= 0x40,  // data (unconfirmed) up
	HDR_FTYPE_DADN		= 0x60,  // data (unconfirmed) dn
	HDR_FTYPE_DCUP		= 0x80,  // data confirmed up
	HDR_FTYPE_DCDN		= 0xA0,  // data confirmed dn
	HDR_FTYPE_REJOIN	= 0xC0,  // rejoin for roaming
	HDR_FTYPE_PROP		= 0xE0
};
enum {
	HDR_MAJOR_V1 = 0x00,
};
enum {
	// Bitfields in frame control octet
	FCT_ADREN	= 0x80,
	FCT_ADRARQ	= 0x40,
	FCT_ACK		= 0x20,
	FCT_MORE	= 0x10,   // also in DN direction: Class B indicator
	FCT_OPTLEN	= 0x0F,
};
enum {
	// In UP direction: signals class B enabled
	FCT_CLASSB = FCT_MORE
};
enum {
	NWKID_MASK = (int)0xFE000000,
	NWKID_BITS = 7
};

// MAC uplink commands   downwlink too
enum {
	// Class A
	MCMD_LCHK_REQ	= 0x02, // -  link check request : -
	MCMD_LADR_ANS	= 0x03, // -  link ADR answer    : u1:7-3:RFU, 3/2/1: pow/DR/Ch ACK
	MCMD_DCAP_ANS	= 0x04, // -  duty cycle answer  : -
	MCMD_DN2P_ANS	= 0x05, // -  2nd DN slot status : u1:7-2:RFU  1/0:datarate/channel ack
	MCMD_DEVS_ANS	= 0x06, // -  device status ans  : u1:battery 0,1-254,255=?, u1:7-6:RFU,5-0:margin(-32..31)
	MCMD_SNCH_ANS	= 0x07, // -  set new channel    : u1: 7-2=RFU, 1/0:DR/freq ACK
	// Class B
	MCMD_PING_IND	= 0x10, // -  pingability indic  : u1: 7=RFU, 6-4:interval, 3-0:datarate
	MCMD_PING_ANS	= 0x11, // -  ack ping freq      : u1: 7-1:RFU, 0:freq ok
	MCMD_BCNI_REQ	= 0x12, // -  next beacon start  : -
};

// MAC downlink commands
enum {
	// Class A
	MCMD_LCHK_ANS	= 0x02, // link check answer  : u1:margin 0-254,255=unknown margin / u1:gwcnt
	MCMD_LADR_REQ	= 0x03, // link ADR request   : u1:DR/TXPow, u2:chmask, u1:chpage/repeat
	MCMD_DCAP_REQ	= 0x04, // duty cycle cap     : u1:255 dead [7-4]:RFU, [3-0]:cap 2^-k
	MCMD_DN2P_SET	= 0x05, // 2nd DN window param: u1:7-4:RFU/3-0:datarate, u3:freq
	MCMD_DEVS_REQ	= 0x06, // device status req  : -
	MCMD_SNCH_REQ	= 0x07, // set new channel    : u1:chidx, u3:freq, u1:DRrange
	// Class B
	MCMD_PING_SET	= 0x11, // set ping freq      : u3: freq
	MCMD_BCNI_ANS	= 0x12, // next beacon start  : u2: delay(in TUNIT millis), u1:channel
};

enum {
	MCMD_BCNI_TUNIT = 30  // time unit of delay value in millis
};
enum {
	MCMD_LADR_ANS_RFU	= 0xF8, // RFU bits
	MCMD_LADR_ANS_POWACK	= 0x04, // 0=not supported power level
	MCMD_LADR_ANS_DRACK	= 0x02, // 0=unknown data rate
	MCMD_LADR_ANS_CHACK	= 0x01, // 0=unknown channel enabled
};
enum {
	MCMD_DN2P_ANS_RFU	= 0xFC, // RFU bits
	MCMD_DN2P_ANS_DRACK	= 0x02, // 0=unknown data rate
	MCMD_DN2P_ANS_CHACK	= 0x01, // 0=unknown channel enabled
};
enum {
	MCMD_SNCH_ANS_RFU	= 0xFC, // RFU bits
	MCMD_SNCH_ANS_DRACK	= 0x02, // 0=unknown data rate
	MCMD_SNCH_ANS_FQACK	= 0x01, // 0=rejected channel frequency
	};
enum {
	MCMD_PING_ANS_RFU	= 0xFE,
	MCMD_PING_ANS_FQACK	= 0x01
};

enum {
	MCMD_DEVS_EXT_POWER	= 0x00, // external power supply
	MCMD_DEVS_BATT_MIN	= 0x01, // min battery value
	MCMD_DEVS_BATT_MAX	= 0xFE, // max battery value
	MCMD_DEVS_BATT_NOINFO 	= 0xFF, // unknown battery level
};

// Bit fields byte#3 of MCMD_LADR_REQ payload
enum {
	MCMD_LADR_CHP_125ON	= 0x60,  // special channel page enable, bits applied to 64..71
	MCMD_LADR_CHP_125OFF	= 0x70,  //  ditto
	MCMD_LADR_N3RFU_MASK	= 0x80,
	MCMD_LADR_CHPAGE_MASK	= 0xF0,
	MCMD_LADR_REPEAT_MASK	= 0x0F,
	MCMD_LADR_REPEAT_1	= 0x01,
	MCMD_LADR_CHPAGE_1	= 0x10
};
// Bit fields byte#0 of MCMD_LADR_REQ payload
enum {
	MCMD_LADR_DR_MASK	= 0xF0,
	MCMD_LADR_POW_MASK	= 0x0F,
	MCMD_LADR_DR_SHIFT	= 4,
	MCMD_LADR_POW_SHIFT	= 0,
#if defined(CFG_eu868)
	MCMD_LADR_SF12		= DR_SF12<<4,
	MCMD_LADR_SF11		= DR_SF11<<4,
	MCMD_LADR_SF10		= DR_SF10<<4,
	MCMD_LADR_SF9		= DR_SF9 <<4,
	MCMD_LADR_SF8		= DR_SF8 <<4,
	MCMD_LADR_SF7		= DR_SF7 <<4,
	MCMD_LADR_SF7B		= DR_SF7B<<4,
	MCMD_LADR_FSK		= DR_FSK <<4,

	MCMD_LADR_20dBm		= 0,
	MCMD_LADR_14dBm		= 1,
	MCMD_LADR_11dBm		= 2,
	MCMD_LADR_8dBm		= 3,
	MCMD_LADR_5dBm		= 4,
	MCMD_LADR_2dBm		= 5,
#elif defined(CFG_us915)
	MCMD_LADR_SF10		= DR_SF10<<4,
	MCMD_LADR_SF9		= DR_SF9 <<4,
	MCMD_LADR_SF8		= DR_SF8 <<4,
	MCMD_LADR_SF7		= DR_SF7 <<4,
	MCMD_LADR_SF8C		= DR_SF8C<<4,
	MCMD_LADR_SF12CR	= DR_SF12CR<<4,
	MCMD_LADR_SF11CR	= DR_SF11CR<<4,
	MCMD_LADR_SF10CR	= DR_SF10CR<<4,
	MCMD_LADR_SF9CR		= DR_SF9CR<<4,
	MCMD_LADR_SF8CR		= DR_SF8CR<<4,
	MCMD_LADR_SF7CR		= DR_SF7CR<<4,

	MCMD_LADR_30dBm		= 0,
	MCMD_LADR_28dBm		= 1,
	MCMD_LADR_26dBm		= 2,
	MCMD_LADR_24dBm		= 3,
	MCMD_LADR_22dBm		= 4,
	MCMD_LADR_20dBm		= 5,
	MCMD_LADR_18dBm		= 6,
	MCMD_LADR_16dBm		= 7,
	MCMD_LADR_14dBm		= 8,
	MCMD_LADR_12dBm		= 9,
	MCMD_LADR_10dBm		= 10
#endif
};

struct lmic_t {
	// Radio settings TX/RX (also accessed by HAL)
	ostime_t	txend;
	ostime_t	rxtime;
	uint32_t	freq;
	int8_t		rssi;
	int8_t		snr;
	uint8_t		rxsyms;
	uint8_t		dndr;
	int8_t		txpow;	// dBm

	uint16_t	opmode;

	uint8_t		ih;
	uint8_t		sf;
	uint8_t		bw;
	uint8_t		cr;
	uint8_t		noCRC;
};
DECLARE_LMIC;


ostime_t os_getTime (void);

void radio_init (void);

void radio_tx(const uint8_t *buffer, size_t len_buffer);
void radio_rx(uint8_t rxmode);
void radio_sleep(void);

//void radio_irq_handler (uint8_t dio);
void radio_irq_handler (uint8_t dio, uint8_t *buffer, size_t *len_buffer);

void os_radio (uint8_t mode);
