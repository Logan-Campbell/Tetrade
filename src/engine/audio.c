/*
* Copyright (c) 2024 Logan Campbell
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#include <stdio.h>
#include "audio.h"

#define DUMMY_BLOCK_ADDR  0x1000
#define BUFFER_START_ADDR 0x1010
#define DUMMY_DATA_SIZE 32
#define ZERO_DATA_SIZE  256
#define STATUS_TIMEOUT  0x100000
#define DMA4_MASK       1 << 24

static int next_channel     = 1;
static int next_sample_addr = BUFFER_START_ADDR;

// Dummy SPU-ADPCM data
// For some reason on real hardware (test on SCPH-7501) when samples
// are played they tend to also play the next sample located in memory
// this dummy data prevents this, when uploaded immediately after a
// real sample.
static const uint8_t dummy_data[] = {
	0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //1st block loop start
	0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  //2nd block loop end+repeat
};

// Copy of _wait_status in psxspu, but waits for DMA4 instead of SPU_STAT
static void _wait_status(uint32_t mask, uint16_t value) {
	for (int i = STATUS_TIMEOUT; i; i--) {
		if ((DMA_CHCR(4) & mask) == value)
			return;
	}

	//_sdk_log("timeout, status=0x%04x\n", SPU_STAT);
}

// Original function "SpuIsTransferCompleted(int mode)" waits for a bit flag
// in SPU_STAT when it should wait for DMA4 to finish when doing DMA transfers.
// More info here: https://psx-spx.consoledev.net/soundprocessingunitspu/#spu-ram-dma-write
// This only causes a problem  when running on real hardware (tested on SCHP-7501), 
// emulators work fine with the original function, when transferinmg via DMA or manually.
int SpuIsTransferCompleted_DMA4(int mode) {
	if (!mode)
		return ((DMA_CHCR(4) >> 24) & 1) ^ 1;
		//return ((SPU_STAT >> 10) & 1) ^ 1;

	_wait_status(DMA4_MASK, 0x0000);
	return 1;
}

int upload_sample(const void *data, int size) {
	// Round the size up to the nearest multiple of 64, as SPU DMA transfers
	// are done in 64-byte blocks.
	int _addr = next_sample_addr;
	int _size = (size + 63) & 0xffffffc0;

	SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
	SpuSetTransferStartAddr(_addr);

	SpuWrite((const uint32_t *) data, _size);
	SpuIsTransferCompleted_DMA4(SPU_TRANSFER_WAIT);

	next_sample_addr = _addr + _size;
	
	return _addr;
}

// Zeros SPU RAM
// When samples get uploaded to spu ram the size of that data is rounded
// up the nearest multiple of 64 (see "upload_sample"), the data between
// the actual size and the uploaded size, will be garbage. This garbage
// can be heard as an annoying tick at the end of every sample, this 
// would only affect real hardware as the spu memory in emulators is
// usually zeroed.
//
// This is a naive solution to clear spu ram and there may be a better
// way of doing it.
void clear_spu_ram(void){

    uint8_t zero_data[ZERO_DATA_SIZE] = {0};

	// Samples are loaded in after this address so no reason to start earlier
	int addr = BUFFER_START_ADDR;

	// Till the end of the reverb work area.
	// https://psx-spx.consoledev.net/soundprocessingunitspu/#spu-memory-layout-512kbyte-ram
	while(addr <= 0x7ffff){

		// Manual write are signifacantly faster the DMA writes
		SpuSetTransferMode(SPU_TRANSFER_BY_IO);
		SpuSetTransferStartAddr(addr);

		SpuWrite((const uint32_t *) zero_data, ZERO_DATA_SIZE);
		
		// We use the original SpuIsTransferCompleted(int mode) function here
		// because we need to wait for a SPU_STAT bit flag when doing manual
		// writes, if psn00bSDK updates in the future it will break (as of 03/2024)
		SpuIsTransferCompleted(SPU_TRANSFER_WAIT);

		addr += ZERO_DATA_SIZE;
	}
};

int play_sample(AudioSample *as) {
	int ch = 0;

	//Find the next silent channel
	for(int i = 0; SPU_CH_ADSR_VOL(i) && i < 24; ++i) {
		ch = i+1;
	}
	
	// Make sure the channel is stopped.
	SpuSetKey(0, 1 << ch);

	SPU_CH_FREQ(ch) = getSPUSampleRate(as->sr);
	SPU_CH_ADDR(ch) = getSPUAddr(as->addr);

	SPU_CH_VOL_L(ch) = as->volume;
	SPU_CH_VOL_R(ch) = as->volume;
	SPU_CH_ADSR1(ch) = 0x00ff;
	SPU_CH_ADSR2(ch) = 0x0000;

	// Start the channel.
	SpuSetKey(1, 1 << ch);

	as->channel = ch;
	return ch;
}

void stop_channel(int channel){
	SpuSetKey(0, 1 << channel);
}

void change_ch_sample_rate(int channel, int sample_rate){
	SPU_CH_FREQ(channel) = getSPUSampleRate(sample_rate);
}

void init_sample_byte(AudioSample *sample, const uint8_t *data) {
	sample->header = (VAG_Header *) data;
    sample->addr = upload_sample(&(sample->header[1]), SWAP_ENDIAN_32(sample->header->size));

	// Upload silent looping dummy data immediatly after sample
	upload_sample(dummy_data, DUMMY_DATA_SIZE);
	
    sample->sr = SWAP_ENDIAN_32(sample->header->sample_rate);
}

void init_sample_vag(AudioSample *sample, VAG_Header *data) {
	sample->header = data;
    sample->addr = upload_sample(&(sample->header[1]), SWAP_ENDIAN_32(sample->header->size));
    sample->sr = SWAP_ENDIAN_32(sample->header->sample_rate);

}

void init_audio(void) {
	clear_spu_ram();
	SpuInit();
}