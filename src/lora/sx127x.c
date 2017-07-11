/*
 * Copyright (c) 2017, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>

#include "sx127x.h"
#include "sx127x_hal.h"

struct lmic_t LMIC;

ostime_t os_getTime (void) {
	return hal_ticks();
}

// RADIO STATE
// (initialized by radio_init(), used by radio_rand1())
static uint8_t randbuf[16];

static void writeReg (uint8_t addr, uint8_t data ) {
	hal_pin_nss(0);
	hal_spi(addr | 0x80);
	hal_spi(data);
	hal_pin_nss(1);
}

static uint8_t readReg (uint8_t addr) {
	hal_pin_nss(0);
	hal_spi(addr & 0x7F);
	uint8_t val = hal_spi(0x00);
	hal_pin_nss(1);
	return val;
}

static void writeBuf (uint8_t addr, uint8_t* buf, uint8_t len) {
	hal_pin_nss(0);
	hal_spi(addr | 0x80);
	for (uint8_t i=0; i<len; i++) {
		hal_spi(buf[i]);
	}
	hal_pin_nss(1);
}

static void readBuf (uint8_t addr, uint8_t* buf, uint8_t len) {
	hal_pin_nss(0);
	hal_spi(addr & 0x7F);
	for (uint8_t i=0; i<len; i++) {
		buf[i] = hal_spi(0x00);
	}
	hal_pin_nss(1);
}

static void opmode (uint8_t mode) {
	writeReg(RegOpMode, (readReg(RegOpMode) & ~OPMODE_MASK) | mode);
}

static void opmodeLora (void) {
	uint8_t u = OPMODE_LORA;
#ifdef CFG_sx1276_radio
	u |= 0x8;	// TBD: sx1276 high freq
#endif
	writeReg(RegOpMode, u);
}

static void opmodeFSK (void) {
    uint8_t u = 0;
#ifdef CFG_sx1276_radio
	u |= 0x8;	// TBD: sx1276 high freq
#endif
	writeReg(RegOpMode, u);
}

// configure LoRa modem (cfg1, cfg2)
static void configLoraModem (void) {
	sf_t sf = LMIC.sf;

#ifdef CFG_sx1276_radio
	uint8_t mc1 = 0, mc2 = 0, mc3 = 0;

	switch (LMIC.bw) {
	case BW125: mc1 |= SX1276_MC1_BW_125; break;
	case BW250: mc1 |= SX1276_MC1_BW_250; break;
	case BW500: mc1 |= SX1276_MC1_BW_500; break;
	default:
		//ASSERT(0);
	}
	switch(LMIC.cr) {
	case CR_4_5: mc1 |= SX1276_MC1_CR_4_5; break;
	case CR_4_6: mc1 |= SX1276_MC1_CR_4_6; break;
	case CR_4_7: mc1 |= SX1276_MC1_CR_4_7; break;
	case CR_4_8: mc1 |= SX1276_MC1_CR_4_8; break;
	default:
		//ASSERT(0);
	}

	if (LMIC.ih) {
		mc1 |= SX1276_MC1_IMPLICIT_HEADER_MODE_ON;
		writeReg(LORARegPayloadLength, LMIC.ih); // required length
	}
	// set ModemConfig1
	writeReg(LORARegModemConfig1, mc1);

	mc2 = (SX1272_MC2_SF7 + ((sf-1)<<4));
	if (LMIC.noCRC == 0) {
		mc2 |= SX1276_MC2_RX_PAYLOAD_CRCON;
	}
	writeReg(LORARegModemConfig2, mc2);

	mc3 = SX1276_MC3_AGCAUTO;
	if ((sf == SF11 || sf == SF12) && LMIC.bw == BW125) {
		mc3 |= SX1276_MC3_LOW_DATA_RATE_OPTIMIZE;
	}
	writeReg(LORARegModemConfig3, mc3);
#elif CFG_sx1272_radio
	uint8_t mc1 = (LMIC.bw<<6);

	switch(LMIC.cr) {
	case CR_4_5: mc1 |= SX1272_MC1_CR_4_5; break;
	case CR_4_6: mc1 |= SX1272_MC1_CR_4_6; break;
	case CR_4_7: mc1 |= SX1272_MC1_CR_4_7; break;
	case CR_4_8: mc1 |= SX1272_MC1_CR_4_8; break;
	}

	if ((sf == SF11 || sf == SF12) && LMIC.bw == BW125) {
		mc1 |= SX1272_MC1_LOW_DATA_RATE_OPTIMIZE;
	}

	if (LMIC.noCRC == 0) {
		mc1 |= SX1272_MC1_RX_PAYLOAD_CRCON;
	}

	if (LMIC.ih) {
		mc1 |= SX1272_MC1_IMPLICIT_HEADER_MODE_ON;
		writeReg(LORARegPayloadLength, LMIC.ih); // required length
	}
	// set ModemConfig1
	writeReg(LORARegModemConfig1, mc1);

	// set ModemConfig2 (sf, AgcAutoOn=1 SymbTimeoutHi=00)
	writeReg(LORARegModemConfig2, (SX1272_MC2_SF7 + ((sf-1)<<4)) | 0x04);

#if CFG_TxContinuousMode
	//Only for testing
	//set ModemConfig2(sf, TxContinuousMode=1, AgcAutoOn=1 SymbTimeoutHi=00)
	writeReg(LORARegModemConfig2, (SX1272_MC2_SF7 + ((sf-1)<<4)) | 0x06);
#endif

#else
//#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif /* CFG_sx1272_radio */
}

static void configChannel (void) {
	// set frequency: FQ = (FRF * 32 Mhz) / (2 ^ 19)
	uint64_t frf = ((uint64_t)LMIC.freq << 19) / 32000000;
	writeReg(RegFrfMsb, (uint8_t)(frf>>16));
	writeReg(RegFrfMid, (uint8_t)(frf>> 8));
	writeReg(RegFrfLsb, (uint8_t)(frf>> 0));
}

static void configPower (void) {
#ifdef CFG_sx1276_radio
	// no boost used for now
	int8_t pw = (int8_t)LMIC.txpow;
	if(pw >= 17) {
		pw = 15;
	} else if(pw < 2) {
		pw = 2;
	}
	// check board type for BOOST pin
	writeReg(RegPaConfig, (uint8_t)(0x80|(pw&0xf)));
	writeReg(RegPaDac, readReg(RegPaDac)|0x4);

#elif CFG_sx1272_radio
	// set PA config (2-17 dBm using PA_BOOST)
	int8_t pw = (int8_t)LMIC.txpow;
	if(pw > 17) {
		pw = 17;
	} else if(pw < 2) {
		pw = 2;
	}
	writeReg(RegPaConfig, (uint8_t)(0x80|(pw-2)));
#else
//#error Missing CFG_sx1272_radio/CFG_sx1276_radio
#endif /* CFG_sx1272_radio */
}

static void txfsk (const uint8_t *buffer, size_t len_buffer) {
	// select FSK modem (from sleep mode)
	writeReg(RegOpMode, 0x10); // FSK, BT=0.5
	//ASSERT(readReg(RegOpMode) == 0x10);
	// enter standby mode (required for FIFO loading))
	opmode(OPMODE_STANDBY);
	// set bitrate
	writeReg(FSKRegBitrateMsb, 0x02); // 50kbps
	writeReg(FSKRegBitrateLsb, 0x80);
	// set frequency deviation
	writeReg(FSKRegFdevMsb, 0x01); // +/- 25kHz
	writeReg(FSKRegFdevLsb, 0x99);
	// frame and packet handler settings
	writeReg(FSKRegPreambleMsb, 0x00);
	writeReg(FSKRegPreambleLsb, 0x05);
	writeReg(FSKRegSyncConfig, 0x12);
	writeReg(FSKRegPacketConfig1, 0xD0);
	writeReg(FSKRegPacketConfig2, 0x40);
	writeReg(FSKRegSyncValue1, 0xC1);
	writeReg(FSKRegSyncValue2, 0x94);
	writeReg(FSKRegSyncValue3, 0xC1);
	// configure frequency
	configChannel();
	// configure output power
	configPower();

	// set the IRQ mapping DIO0=PacketSent DIO1=NOP DIO2=NOP
	writeReg(RegDioMapping1, MAP_DIO0_FSK_READY|
					MAP_DIO1_FSK_NOP|MAP_DIO2_FSK_TXNOP);

	// initialize the payload size and address pointers
	// (insert length byte into payload))
	writeReg(FSKRegPayloadLength, len_buffer+1);

	// download length byte and buffer to the radio FIFO
	writeReg(RegFifo, len_buffer);
	writeBuf(RegFifo, buffer, len_buffer);

	// enable antenna switch for TX
	hal_pin_rxtx(1);

	// now we actually start the transmission
	opmode(OPMODE_TX);
}

static void txlora (const uint8_t *buffer, size_t len_buffer) {
	// select LoRa modem (from sleep mode)
	//writeReg(RegOpMode, OPMODE_LORA);
	opmodeLora();
	//ASSERT((readReg(RegOpMode) & OPMODE_LORA) != 0);

	// enter standby mode (required for FIFO loading))
	opmode(OPMODE_STANDBY);
	// configure LoRa modem (cfg1, cfg2)
	configLoraModem();
	// configure frequency
	configChannel();
	// configure output power
	// set PA ramp-up time 50 uSec
	writeReg(RegPaRamp, (readReg(RegPaRamp) & 0xF0) | 0x08);
	configPower();
	// set sync word
	writeReg(LORARegSyncWord, LORA_MAC_PREAMBLE);

	// set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
	writeReg(RegDioMapping1, MAP_DIO0_LORA_TXDONE|MAP_DIO1_LORA_NOP|
							MAP_DIO2_LORA_NOP);
	// clear all radio IRQ flags
	writeReg(LORARegIrqFlags, 0xFF);
	// mask all IRQs but TxDone
	writeReg(LORARegIrqFlagsMask, ~IRQ_LORA_TXDONE_MASK);

	// initialize the payload size and address pointers
	writeReg(LORARegFifoTxBaseAddr, 0x00);
	writeReg(LORARegFifoAddrPtr, 0x00);
	writeReg(LORARegPayloadLength, len_buffer);

	// download buffer to the radio FIFO
	writeBuf(RegFifo, buffer, len_buffer);

	// enable antenna switch for TX
	hal_pin_rxtx(1);

	// now we actually start the transmission
	opmode(OPMODE_TX);
}

// start transmitter (buf=LMIC.frame, len=LMIC.dataLen)
static void starttx (const uint8_t *buffer, size_t len_buffer) {
	//ASSERT( (readReg(RegOpMode) & OPMODE_MASK) == OPMODE_SLEEP );
	if(LMIC.sf == FSK) { // FSK modem
		txfsk(buffer, len_buffer);
	} else { // LoRa modem
		txlora(buffer, len_buffer);
	}
	// the radio will go back to STANDBY mode as soon as the TX is finished
	// the corresponding IRQ will inform us about completion.
	}

static const uint8_t rxlorairqmask[] = {
	[RXMODE_SINGLE]	= IRQ_LORA_RXDONE_MASK|IRQ_LORA_RXTOUT_MASK,
	[RXMODE_SCAN]	= IRQ_LORA_RXDONE_MASK,
	[RXMODE_RSSI]	= 0x00,
};

// start LoRa receiver
//(time=LMIC.rxtime, timeout=LMIC.rxsyms, result=LMIC.frame[LMIC.dataLen])
static void rxlora (uint8_t rxmode) {
	// select LoRa modem (from sleep mode)
	opmodeLora();
	//ASSERT((readReg(RegOpMode) & OPMODE_LORA) != 0);
	// enter standby mode (warm up))
	opmode(OPMODE_STANDBY);
	// don't use MAC settings at startup
	if(rxmode == RXMODE_RSSI) { // use fixed settings for rssi scan
		writeReg(LORARegModemConfig1,
					RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG1);
		writeReg(LORARegModemConfig2,
					RXLORA_RXMODE_RSSI_REG_MODEM_CONFIG2);
	} else { // single or continuous rx mode
	// configure LoRa modem (cfg1, cfg2)
		configLoraModem();
	// configure frequency
		configChannel();
	}
	// set LNA gain
	writeReg(RegLna, LNA_RX_GAIN);
	// set max payload size
	writeReg(LORARegPayloadMaxLength, 64);
	// use inverted I/Q signal (prevent mote-to-mote communication)
/*
	// XXX: use flag to switch on/off inversion
	if (LMIC.noRXIQinversion) {
		writeReg(LORARegInvertIQ, readReg(LORARegInvertIQ) & ~(1<<6));
	} else {
		writeReg(LORARegInvertIQ, readReg(LORARegInvertIQ)|(1<<6));
	}
*/
#if !defined(DISABLE_INVERT_IQ_ON_RX)
	// use inverted I/Q signal (prevent mote-to-mote communication)
	writeReg(LORARegInvertIQ, readReg(LORARegInvertIQ)|(1<<6));
#endif

	// set symbol timeout (for single rx)
	writeReg(LORARegSymbTimeoutLsb, LMIC.rxsyms);
	// set sync word
	writeReg(LORARegSyncWord, LORA_MAC_PREAMBLE);

	// configure DIO mapping DIO0=RxDone DIO1=RxTout DIO2=NOP
	writeReg(RegDioMapping1, MAP_DIO0_LORA_RXDONE|MAP_DIO1_LORA_RXTOUT|
							MAP_DIO2_LORA_NOP);
	// clear all radio IRQ flags
	writeReg(LORARegIrqFlags, 0xFF);
	// enable required radio IRQs
	writeReg(LORARegIrqFlagsMask, ~rxlorairqmask[rxmode]);

	// enable antenna switch for RX
	hal_pin_rxtx(0);

	// now instruct the radio to receive
	if (rxmode == RXMODE_SINGLE) { // single rx
		hal_waitUntil(LMIC.rxtime); // busy wait until exact rx time
		opmode(OPMODE_RX_SINGLE);
	} else { // continous rx (scan or rssi)
		opmode(OPMODE_RX);
	}
}

static void rxfsk (uint8_t rxmode) {
	// only single rx (no continuous scanning, no noise sampling)
	//ASSERT( rxmode == RXMODE_SINGLE );
	// select FSK modem (from sleep mode)
	//writeReg(RegOpMode, 0x00); // (not LoRa)
	opmodeFSK();
	//ASSERT((readReg(RegOpMode) & OPMODE_LORA) == 0);
	// enter standby mode (warm up))
	opmode(OPMODE_STANDBY);
	// configure frequency
	configChannel();
	// set LNA gain
	//writeReg(RegLna, 0x20|0x03); // max gain, boost enable
	writeReg(RegLna, LNA_RX_GAIN);
	// configure receiver
	writeReg(FSKRegRxConfig, 0x1E); // AFC auto, AGC, trigger on preamble?!?
	// set receiver bandwidth
	writeReg(FSKRegRxBw, 0x0B); // 50kHz SSb
	// set AFC bandwidth
	writeReg(FSKRegAfcBw, 0x12); // 83.3kHz SSB
	// set preamble detection
	writeReg(FSKRegPreambleDetect, 0xAA); // enable, 2 bytes, 10 chip errors
	// set sync config
	// no auto restart, preamble 0xAA, enable, fill FIFO, 3 bytes sync
	writeReg(FSKRegSyncConfig, 0x12);
	// set packet config
	// var-length, whitening, crc, no auto-clear, no adr filter
	writeReg(FSKRegPacketConfig1, 0xD8);
	writeReg(FSKRegPacketConfig2, 0x40); // packet mode
	// set sync value
	writeReg(FSKRegSyncValue1, 0xC1);
	writeReg(FSKRegSyncValue2, 0x94);
	writeReg(FSKRegSyncValue3, 0xC1);
	// set preamble timeout
	writeReg(FSKRegRxTimeout2, 0xFF);//(LMIC.rxsyms+1)/2);
	// set bitrate
	writeReg(FSKRegBitrateMsb, 0x02); // 50kbps
	writeReg(FSKRegBitrateLsb, 0x80);
	// set frequency deviation
	writeReg(FSKRegFdevMsb, 0x01); // +/- 25kHz
	writeReg(FSKRegFdevLsb, 0x99);

	// configure DIO mapping DIO0=PayloadReady DIO1=NOP DIO2=TimeOut
	writeReg(RegDioMapping1, MAP_DIO0_FSK_READY|MAP_DIO1_FSK_NOP|
							MAP_DIO2_FSK_TIMEOUT);

	// enable antenna switch for RX
	hal_pin_rxtx(0);

	// now instruct the radio to receive
	hal_waitUntil(LMIC.rxtime); // busy wait until exact rx time
	opmode(OPMODE_RX); // no single rx mode available in FSK
	}

static void startrx (uint8_t rxmode) {
	//ASSERT( (readReg(RegOpMode) & OPMODE_MASK) == OPMODE_SLEEP );
	if(LMIC.sf == FSK) { // FSK modem
		rxfsk(rxmode);
	} else { // LoRa modem
		rxlora(rxmode);
	}
	// the radio will go back to STANDBY mode as soon as the RX is finished
	// or timed out, and the corresponding IRQ will
	//inform us about completion.
	}

// get random seed from wideband noise rssi
void radio_init () {
	hal_disableIRQs();

	// manually reset radio
#ifdef CFG_sx1276_radio
	hal_pin_rst(0); // drive RST pin low
#else
	hal_pin_rst(1); // drive RST pin high
#endif
	hal_waitUntil(os_getTime()+ms2osticks(1)); // wait >100us
	hal_pin_rst(2); // configure RST pin floating!
	hal_waitUntil(os_getTime()+ms2osticks(5)); // wait 5ms

	opmode(OPMODE_SLEEP);

	// some sanity checks, e.g., read version number
	uint8_t v = readReg(RegVersion);

	// seed 15-byte randomness via noise rssi
	rxlora(RXMODE_RSSI);
	while( (readReg(RegOpMode) & OPMODE_MASK) != OPMODE_RX );
	for(int i=1; i<16; i++) {
		for(int j=0; j<8; j++) {
		// wait for two non-identical subsequent least-significant bits
			uint8_t b;
			while( (b = readReg(LORARegRssiWideband) & 0x01) ==
					(readReg(LORARegRssiWideband) & 0x01) );
			randbuf[i] = (randbuf[i] << 1) | b;
		}
	}
	randbuf[0] = 16; // set initial index

#ifdef CFG_sx1276mb1_board
	// chain calibration
	writeReg(RegPaConfig, 0);

	// Launch Rx chain calibration for LF band
	writeReg(FSKRegImageCal, (readReg(FSKRegImageCal) &
			 RF_IMAGECAL_IMAGECAL_MASK)|RF_IMAGECAL_IMAGECAL_START);
	while((readReg(FSKRegImageCal)&RF_IMAGECAL_IMAGECAL_RUNNING) ==
					RF_IMAGECAL_IMAGECAL_RUNNING){ ; }

	// Sets a Frequency in HF band
	uint32_t frf = 868000000;
	writeReg(RegFrfMsb, (uint8_t)(frf>>16));
	writeReg(RegFrfMid, (uint8_t)(frf>> 8));
	writeReg(RegFrfLsb, (uint8_t)(frf>> 0));

	// Launch Rx chain calibration for HF band
	writeReg(FSKRegImageCal, (readReg(FSKRegImageCal) &
			RF_IMAGECAL_IMAGECAL_MASK)|RF_IMAGECAL_IMAGECAL_START);
	while((readReg(FSKRegImageCal) & RF_IMAGECAL_IMAGECAL_RUNNING) ==
					RF_IMAGECAL_IMAGECAL_RUNNING) { ; }
#endif /* CFG_sx1276mb1_board */

	opmode(OPMODE_SLEEP);

	hal_enableIRQs();
}

static const uint16_t LORA_RXDONE_FIXUP[] = {
	[FSK]  =     us2osticks(0), // (   0 ticks)
	[SF7]  =     us2osticks(0), // (   0 ticks)
	[SF8]  =  us2osticks(1648), // (  54 ticks)
	[SF9]  =  us2osticks(3265), // ( 107 ticks)
	[SF10] =  us2osticks(7049), // ( 231 ticks)
	[SF11] = us2osticks(13641), // ( 447 ticks)
	[SF12] = us2osticks(31189), // (1022 ticks)
};

// called by hal ext IRQ handler
// (radio goes to stanby mode after tx/rx operations)
void radio_irq_handler (uint8_t dio, uint8_t *buffer, size_t *len_buffer) {
#if CFG_TxContinuousMode
	// clear radio IRQ flags
	writeReg(LORARegIrqFlags, 0xFF);
	uint8_t p = readReg(LORARegFifoAddrPtr);
	writeReg(LORARegFifoAddrPtr, 0x00);
	uint8_t s = readReg(RegOpMode);
	uint8_t c = readReg(LORARegModemConfig2);
	opmode(OPMODE_TX);
	return;
#else /* ! CFG_TxContinuousMode */
	ostime_t now = os_getTime();
	if( (readReg(RegOpMode) & OPMODE_LORA) != 0) { // LORA modem
		uint8_t flags = readReg(LORARegIrqFlags);
	if( flags & IRQ_LORA_TXDONE_MASK ) {
		// save exact tx time
		LMIC.txend = now - us2osticks(43); // TXDONE FIXUP
		} else if( flags & IRQ_LORA_RXDONE_MASK ) {
			// save exact rx time
			if(LMIC.bw == BW125) {
				now -= LORA_RXDONE_FIXUP[LMIC.sf];
			}
			LMIC.rxtime = now;
			// read the PDU and inform the MAC that we received something
			*len_buffer = (readReg(LORARegModemConfig1) &
					SX1272_MC1_IMPLICIT_HEADER_MODE_ON) ?
					readReg(LORARegPayloadLength) :
					readReg(LORARegRxNbBytes);
			// set FIFO read address pointer
			writeReg(LORARegFifoAddrPtr,
					readReg(LORARegFifoRxCurrentAddr));
			// now read the FIFO
			readBuf(RegFifo, buffer, *len_buffer);
			// read rx quality parameters
			// SNR [dB] * 4
			LMIC.snr  = readReg(LORARegPktSnrValue);
			// RSSI [dBm] (-196...+63)
			LMIC.rssi = readReg(LORARegPktRssiValue) - 125 + 64;
		} else if( flags & IRQ_LORA_RXTOUT_MASK ) {
			// indicate timeout
			*len_buffer = 0;
		}
		// mask all radio IRQs
		writeReg(LORARegIrqFlagsMask, 0xFF);
		// clear radio IRQ flags
		writeReg(LORARegIrqFlags, 0xFF);
	} else { // FSK modem
	uint8_t flags1 = readReg(FSKRegIrqFlags1);
	uint8_t flags2 = readReg(FSKRegIrqFlags2);
	if( flags2 & IRQ_FSK2_PACKETSENT_MASK ) {
		// save exact tx time
		LMIC.txend = now;
	} else if( flags2 & IRQ_FSK2_PAYLOADREADY_MASK ) {
		// save exact rx time
		LMIC.rxtime = now;
		// read the PDU and inform the MAC that we received something
		*len_buffer = readReg(FSKRegPayloadLength);
		// now read the FIFO
		readBuf(RegFifo, buffer, *len_buffer);
		// read rx quality parameters
		LMIC.snr  = 0; // determine snr
		LMIC.rssi = 0; // determine rssi
	} else if( flags1 & IRQ_FSK1_TIMEOUT_MASK ) {
		// indicate timeout
		*len_buffer = 0;
	} else {
		while(1);
	}
	}
	// go from stanby to sleep
	opmode(OPMODE_SLEEP);
#endif /* ! CFG_TxContinuousMode */
}

int radio_irq_flag (uint8_t mask)
{
	uint8_t flags = readReg(LORARegIrqFlags);

	if ((flags & mask) == mask)
		return 0;

	return 1;
}

void radio_set_config(uint32_t freq, int8_t txpow, uint8_t sf, uint8_t bw,
					uint8_t cr, uint8_t ih, uint8_t noCRC)
{
	LMIC.freq	= freq;
	LMIC.txpow	= txpow;
	LMIC.sf		= sf;
	LMIC.bw		= bw;
	LMIC.cr		= cr;
	LMIC.ih		= ih;
	LMIC.noCRC	= noCRC;
}

void radio_tx(const uint8_t *buffer, size_t len_buffer)
{
	hal_disableIRQs();
	starttx(bconst uffer, len_buffer);
	hal_enableIRQs();
}

void radio_rx(uint8_t rxmode)
{
	hal_disableIRQs();
	startrx(rxmode);
	hal_enableIRQs();
}

void radio_sleep(void)
{
	hal_disableIRQs();
	opmode(OPMODE_SLEEP);
	hal_enableIRQs();
}
