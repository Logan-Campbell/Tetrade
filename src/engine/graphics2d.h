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
#include <stdio.h>
#include <stdlib.h>
#include <psxgte.h>
#include <psxgpu.h>
#include <psxcd.h>
#include "fpmath.h"
#include "timer.h"

#define DEBUG_MODE 1
#define PAL_MODE 0

#if PAL_MODE
    #define SCREEN_WIDTH 320
    #define SCREEN_HEIGHT 256
#else
    #define SCREEN_WIDTH 320
    #define SCREEN_HEIGHT 240
#endif

typedef struct _Sprite {
    unsigned short tpage;   //tpage
    unsigned short clut;    //clut
    unsigned char u, v;     //VRAM coords
    unsigned char w, h;     //width, height
    int x, y;
    int xscale, yscale;
    int angle;
    CVECTOR color;
} Sprite;

typedef struct _AnimatedSprite {
    Sprite *spriteList; //List of sprites in animation
    int numFrames;      //total number of frames (size of spriteList)
    int frame;          //current frame (0 index)
    int animation_rate; 
    int x, y;           //Position

} AnimatedSprite;


#define OTLEN 8

typedef struct _FrameBuffer {
    DISPENV disp[2];
    DRAWENV draw[2];
} FrameBuffer;

typedef struct _RenderContext{
    FrameBuffer db;
    int db_active;
    uint32_t ot[2][OTLEN];
    char primbuff[2][32768];
    char *nextpri;
} RenderContext;

#if DEBUG_MODE 
extern int fnt;
#endif

// Places external tim image into VRam and loads params into tparam.
void load_texture(uint32_t *tim, TIM_IMAGE *tparam);

char *load_file(const char *filename);

// Loads file from the CD into VRam and loads params into tparam.
void load_cd_texture(uint32_t *tim, TIM_IMAGE *tparam, const char *filename);

// Loads tim into Sprite struct.
void load_sprite(Sprite *sprite, TIM_IMAGE *tim);

// Loads sprites in a sprite sheet in to a list.
// spriteList Sprite array to store sprites
// sH Individual Sprite Height
// sW Individual Sprite Width
// sNum Number of sprites in sheet
// numCol Number of columns of the sprite sheet
void load_sprite_sheet(Sprite* spriteList, const int sH, const int sW, const int sNum, const int numCol, TIM_IMAGE *tim);

void rotate_sprite(Sprite *sprite, const int angle);

void move_sprite(Sprite *sprite, const int x, const int y);

void scale_sprite(Sprite *sprite, const int xScale, const int yScale);

// Adds sprite into ordering table to be drawn.
void draw_sprite(Sprite *sprite);

// Draw primitive tile
void draw_tile(const CVECTOR color, const int x, const int y, const int w, const int h);

// Draw primitive line
void draw_line(const CVECTOR color, const int x0, const int y0, const int x1, const int y1);

// Advance one frame and draw the sprite;
void animate(AnimatedSprite *animSprite);

void change_bkg_color(const CVECTOR color);

// Draws ordering table, flips buffer
void display(void);

// Intitialzies draw env and buffers
void init_gfx(void);


void init_debug_fnt(void);