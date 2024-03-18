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

#include "text.h"
#include <string.h>

static void _draw_character(TextSprite *textSprite, const int x, const int y, const int c) {
    if(c == -1) return;
    Sprite sprite = textSprite->spritesList[c];
    sprite.x = x;
    sprite.y = y;
    draw_sprite(&sprite);
}

void load_text(TextSprite *textSprite, const char *charList, TIM_IMAGE *textSheet,
                const int w, const int h, const int length) {
    
    textSprite->size = length;
    
    textSprite->characterList = malloc(sizeof(char)*length);
    memcpy(textSprite->characterList, charList, length);

    textSprite->charW = w;
    textSprite->charH = h;
    textSprite->cols = (textSheet->prect->w<<(2-textSheet->mode&0x3))/w;
    textSprite->rows = textSheet->prect->h/h;

    load_sprite_sheet(textSprite->spritesList, h, w, length, textSprite->cols, textSheet);
}

int print_text(TextSprite *textSprite, const int x, const int y, 
                const char *fmt, ...) {

    int startx = x;
    int starty = y;
    //char* string = fmt;
    char string[128];
    int l;
    va_list ap;

    va_start(ap, fmt);
    l = vsprintf(string, fmt, ap);
    va_end(ap);

    //printf("l: %d, string: %s \n", l, string);
    if(l < 0) return l;

    for(int i = 0; i < l; i++) {
        int characterNum = -1; //, -1 = not found
        char c = string[i];

        if(c == '\n') {
            starty += textSprite->charH;
            continue;
        }

        //Find index of current character in textsprite character list
        for(int j = 0; j < textSprite->size; j++) {
            if(textSprite->characterList[j] == c) {
                characterNum = j;
                //printf("Printing character: %d, %c \n", j, c);
                break;
            }
        }

        _draw_character(textSprite, startx, starty, characterNum);
        startx += textSprite->charW;
    }

    return 0;
}