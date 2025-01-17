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

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <psxetc.h>
#include <psxspu.h>
#include <hwregs_c.h>
#include "audiotypes.h"

#define SWAP_ENDIAN_32(x) ( \
	(((uint32_t) (x) & 0x000000ff) << 24) | \
	(((uint32_t) (x) & 0x0000ff00) <<  8) | \
	(((uint32_t) (x) & 0x00ff0000) >>  8) | \
	(((uint32_t) (x) & 0xff000000) >> 24) \
)

void reset_spu_channels(void);
int upload_sample(const void *data, int size);
int play_sample(AudioSample *sample);

void init_sample_byte(AudioSample *sample, const uint8_t *data);
void init_sample_vag(AudioSample *sample, VAG_Header *data);

void stop_channel(int channel);
void change_ch_sample_rate(int channel, int sample_rate);
void init_audio(void);