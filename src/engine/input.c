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

#include "input.h"

static unsigned char padbuff[2][34];

//store button state of last poll
static uint16_t padStates[2];

static int _get_raw_input(const int port, const int button) {
    // Parse controller input
    PADTYPE* pad = (PADTYPE*)padbuff[port];

    // Only parse inputs when a controller is connected
    if(pad->stat != 0) {
        return 0;
    }

    // Only parse when a digital pad, 
    // dual-analog and dual-shock is connected
    if((pad->type == 0x4) ||
        (pad->type == 0x5) ||
        (pad->type == 0x7)) {
        return !(pad->btn&button);
    }

    return 0;
}


int button_up(const int port, const int button) {
    return (!_get_raw_input(port, button) && !(padStates[port]&button));
}

int button_down(const int port, const int button) {
    return (_get_raw_input(port, button) && padStates[port]&button);
}

int button_pressed(const int port, const int button) {
    return  (_get_raw_input(port, button) && !(padStates[port]&button));
}

void poll_input(const int port) {
    // Parse controller input
    PADTYPE* pad = (PADTYPE*)padbuff[port];

    // Only parse inputs when a controller is connected
    if(pad->stat != 0) {
        return;
    }

    // Only parse when a digital pad, 
    // dual-analog and dual-shock is connected
    if((pad->type == 0x4) ||
        (pad->type == 0x5) ||
        (pad->type == 0x7)) {
        padStates[port] = pad->btn;
    }
}

void init_input(void) {
    InitPAD(padbuff[0], 34, padbuff[1], 34);

    //So controller polling does not return faulty data.
    padbuff[0][0] = padbuff[0][1] = 0xff;
    padbuff[1][0] = padbuff[1][1] = 0xff;

    StartPAD();
    ChangeClearPAD(1); //Avoid "VSync: Timeout" tty message caused bt StartPAD()
}