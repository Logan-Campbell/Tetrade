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

#define FIXED_SCALE 12 //1 sign bit, 19 Decimal Bits, 12 Fractional bits
#define FIXED_ONE 4096
#define FRACTION_MASK 0xfff
#define IntToFixed(x) ((x) << FIXED_SCALE)
#define FixedToInt(x) ((x) >> FIXED_SCALE)
#define MulFixed(x,y) (((x)*(y))>>FIXED_SCALE)
#define DivFixed(x,y) (((x)*FIXED_ONE)/(y))
#define DecPart(x)    ((x) & (-1 ^ FRACTION_MASK))
#define FracPart(x)   ((x) & FRACTION_MASK)