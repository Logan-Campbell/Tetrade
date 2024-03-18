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

#include "timer.h"

volatile static uint32_t systemTime;

static void _timer2_callback(void) {
    systemTime++;
}

void create_timer(Timer *timer) {
    timer->time = 0;
}

void init_system_timer(void) {
    int counter;

    EnterCriticalSection();
	
	// NTSC clock base
	counter = 4304000/560;
	
	// PAL clock base
	//counter = 5163000/560;
	
	SetRCnt(RCntCNT2, counter, RCntMdINTR);
	TIMER_CTRL(2) = 0x0258; // CLK/8 input, IRQ on reload
    TIMER_RELOAD(2)    = (F_CPU / 8) / 1000;    // 100 Hz

    // Configure timer 2 IRQ
	InterruptCallback(IRQ_TIMER2, &_timer2_callback);
	StartRCnt(RCntCNT2);
	ChangeClearRCnt(2, 0);
	ExitCriticalSection();
}

uint32_t get_system_time(void) {
    return systemTime;
}