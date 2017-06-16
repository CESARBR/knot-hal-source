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


void radio_init (void);

void radio_irq_handler (uint8_t dio);

void os_radio (uint8_t mode);