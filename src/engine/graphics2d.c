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

#include "graphics2d.h"

static RenderContext ctx;

// Controls animation timing currently
Timer mainTimer;
int fnt;

void load_texture(uint32_t *tim, TIM_IMAGE *tparam) {
    // Read TIM information (PSn00bSDK)
    GetTimInfo(tim, tparam);

    // Upload pixel data to framebuffer
    LoadImage(tparam->prect, (uint32_t*)tparam->paddr);
    DrawSync(0);

    // Upload CLUT to framebuffer if present
    if(tparam->mode & 0x8) {
        LoadImage(tparam->crect, (uint32_t*)tparam->caddr);
        DrawSync(0);
    }
}

char *load_file(const char *filename) {
    CdlFILE filePos;
    int numsecs;
    char *buff;

    buff = NULL;

    /* locate the file on the CD */
    if(CdSearchFile(&filePos, filename) == NULL) {
        /* print error message if file not found */
        printf("file: %s not found.\n", filename);
    } else {
        /* calculate number of sectors to read for the file */
        numsecs = (filePos.size+2047)/2048;

        /* allocate buffer for the file */
        buff = (char*)malloc(2048*numsecs);

        /* set read target to the file */
        CdControl(CdlSetloc, (unsigned char*)&filePos.pos, 0);

        /* start read operation */
        CdRead(numsecs, (uint32_t*)buff, CdlModeSpeed);

        /* wait until the read operation is complete */
        CdReadSync(0,0);
    }

    return buff;
}

void load_cd_texture(uint32_t *tim, TIM_IMAGE *tparam, const char *filename) {
    uint32_t *filebuff;

    if(filebuff = (uint32_t*)load_file(filename)) {
        load_texture(tim, tparam);

        // Free the file buffer
        free(filebuff);

    } else {
        // Output error text that the image failed to load
        printf("Error: %s file not found.\n", filename);
    }
}

void load_sprite(Sprite *sprite, TIM_IMAGE *tim) {
    // Get tpage value
    sprite->tpage = getTPage(tim->mode&0x3, 0, 
        tim->prect->x, tim->prect->y);

    // Get CLUT value
    if(tim->mode & 0x8) {
        sprite->clut = getClut(tim->crect->x, tim->crect->y);
    }

    // Set sprite size
    sprite->w = tim->prect->w<<(2-tim->mode&0x3);
    sprite->h = tim->prect->h;

    // Set UV offset
    sprite->u = (tim->prect->x&0x3f)<<(2-tim->mode&0x3);
    sprite->v = tim->prect->y&0xff;

    // Set neutral color
    sprite->color.r = 128;
    sprite->color.g = 128;
    sprite->color.b = 128;

    // Set default position
    sprite->x = 0;
    sprite->y = 0;

    // Set default scale
    sprite->xscale = FIXED_ONE;
    sprite->yscale = FIXED_ONE;

    // Set default angle
    sprite->angle = 0;
}

/// @brief 
/// @param spriteList List to store sprites into
/// @param sH         Height of a sprite
/// @param sW         Width of the sprite
/// @param sNum       Number of sprites
/// @param numCol     Number of columns of the sprite sheet
/// @param tim        Sprite sheet image
void load_sprite_sheet(Sprite *spriteList, const int sH, const int sW, const int sNum, const int numCol, TIM_IMAGE *tim) {
    int curCol = 0;
    int curRow = 0;
    const int u = (tim->prect->x&0x3f)<<(2-tim->mode&0x3);
    const int v = tim->prect->y&0xff;
    for(int i = 0; i < sNum; i++) {        
        Sprite sprite;
        load_sprite(&sprite, tim);

        sprite.u = u+(curCol*sW);
        sprite.v = v+(curRow*sH);
        sprite.h = sH;
        sprite.w = sW;
        spriteList[i] = sprite;
        
        curCol++;
        if(curCol >= numCol) {
            curCol = 0;
            curRow++;
        }
    }
}

void rotate_sprite(Sprite *sprite, const int angle) {
    sprite->angle = angle;
}

void move_sprite(Sprite *sprite, const int x, const int y) {
    sprite->x = x;
    sprite->y = y;
}

void scale_sprite(Sprite *sprite, const int xScale, const int yScale) {
    sprite->xscale = xScale;
    sprite->xscale = yScale;
}

// Draw sprite as polygon
static void _draw_rotated_sprite(Sprite *sprite) {
    POLY_FT4 *quad;
    SVECTOR	s[4];
    SVECTOR	v[4];

    int x,y,pw,ph,angle;
    x = sprite->x;
    y = sprite->y;
    pw = sprite->w;
    ph = sprite->h;
    angle = sprite->angle;
    int i,cx,cy;

    // calculate the pivot point (center) of the sprite
    cx = pw>>1;
    cy = ph>>1;

    const int fixedHalf = FIXED_ONE>>1;
    // increment by 0.5 on the bottom and right coords so scaling
    // would increment a bit smoother
    s[0].vx = -(((pw*FIXED_ONE)>>FIXED_SCALE)-cx);
    s[0].vy = -(((ph*FIXED_ONE)>>FIXED_SCALE)-cy);

    s[1].vx = (((pw*FIXED_ONE)+fixedHalf)>>FIXED_SCALE)-cx;
    s[1].vy = s[0].vy;

    s[2].vx = -(((pw*FIXED_ONE)>>FIXED_SCALE)-cx);
    s[2].vy = (((ph*FIXED_ONE)+fixedHalf)>>FIXED_SCALE)-cy;

    s[3].vx = (((pw*FIXED_ONE)+fixedHalf)>>FIXED_SCALE)-cx;
    s[3].vy = s[2].vy;
    
    // a simple but pretty effective optimization trick
    cx = ccos(angle);
    cy = csin(angle);
    
    // calculate rotated sprite coordinates
    for( i=0; i<4; i++ ) {
        v[i].vx = (((s[i].vx*cx)
            -(s[i].vy*cy))>>FIXED_SCALE)+x;
        v[i].vy = (((s[i].vy*cx)
            +(s[i].vx*cy))>>FIXED_SCALE)+y;
    }

    // initialize the quad primitive for the sprite
    quad = (POLY_FT4*)(ctx.nextpri);
    setPolyFT4(quad);

    // set CLUT and tpage to the primitive
    quad->tpage = sprite->tpage;
    quad->clut = sprite->clut;

    // set color, screen coordinates and texture coordinates of primitive
    setRGB0(quad,                       // Set the color
        sprite->color.r,
        sprite->color.g,
        sprite->color.b);
    setXY4(quad,
        v[0].vx, v[0].vy,
        v[1].vx, v[1].vy,
        v[2].vx, v[2].vy,
        v[3].vx, v[3].vy );
    setUVWH(quad, sprite->u, sprite->v, pw, ph);

    // add it to the ordering table
    addPrim( ctx.ot[ctx.db_active], quad );
    ctx.nextpri += sizeof(POLY_FT4); 
}

// Draw sprite as basic SPRT primitive
static void _draw_sprite(Sprite *sprite) {
    uint32_t *ot = ctx.ot[ctx.db_active];

    SPRT *sprt;
    DR_TPAGE *tpage;

    sprt = (SPRT*)(ctx.nextpri);
    setSprt(sprt);

    setRGB0(sprt,                       
        sprite->color.r,
        sprite->color.g,
        sprite->color.b);
    setXY0(sprt, sprite->x, sprite->y);
    setUV0(sprt, sprite->u, sprite->v);
    setWH(sprt, sprite->w, sprite->h);
   
    sprt->clut = sprite->clut;

    // Add it to the ordering table
    addPrim( ot, sprt );
    ctx.nextpri += sizeof(SPRT); 

    // Add tpage
    tpage = (DR_TPAGE*)(ctx.nextpri);            
    setDrawTPage(tpage, 0, 1, sprite->tpage);
    addPrim(ot, tpage);

    ctx.nextpri += sizeof(DR_TPAGE);   
                                  
}

void draw_sprite(Sprite *sprite) {
    // Drawing rotated sprites (ie polygons) are very slow compared to sprite
    // primitives so if there is no rotation to the sprite, then 
    // draw as a primitive sprite.
    if(sprite->angle) {
        _draw_rotated_sprite(sprite);
    } else {
        _draw_sprite(sprite);
    }
}

void draw_tile(const CVECTOR color, const int x, const int y, const int w, const int h) {
    TILE *tile = (TILE*)ctx.nextpri; // Cast next primitive

    setTile(tile);               // Initialize the primitive
    setXY0(tile, x, y);          // Set primitive (x,y) position
    setWH(tile, w, h);           // Set primitive size
    setRGB0(tile, 
        color.r, 
        color.g, 
        color.b);
    addPrim(ctx.ot[ctx.db_active], tile);       // Add primitive to the ordering table
    
    ctx.nextpri += sizeof(TILE); 
}

void draw_line(const CVECTOR color, const int x0, const int y0, const int x1, const int y1) {
    LINE_F2 *line = (LINE_F2*)ctx.nextpri;
    setLineF2(line);
    setXY2(line, x0, y0, x1, y1);
    setRGB0(line, 
        color.r, 
        color.g, 
        color.b);
    addPrim(ctx.ot[ctx.db_active], line);       // Add primitive to the ordering table
    ctx.nextpri += sizeof(LINE_F2); 
}

void animate(AnimatedSprite *animSprite) {
    //Will run slower in PAL mode ie. fix
    if(mainTimer.time % animSprite->animation_rate == 0) {
        animSprite->frame++;
        if(animSprite->frame >= animSprite->numFrames) {
            animSprite->frame = 0;
        }
    }
    
    animSprite->spriteList[animSprite->frame].x = animSprite->x;
    animSprite->spriteList[animSprite->frame].y = animSprite->y;
    draw_sprite(&animSprite->spriteList[animSprite->frame]);
}

void change_bkg_color(const CVECTOR color) {
    FrameBuffer *db = &(ctx.db);

    setRGB0(&(db->draw[0]), color.r, color.g, color.b);
    setRGB0(&(db->draw[1]), color.r, color.g, color.b);
}

void display(void) {
    DrawSync(0);
    VSync(0);

    FrameBuffer *db = &(ctx.db);

    PutDispEnv(&(db->disp[ctx.db_active]));
    PutDrawEnv(&(db->draw[ctx.db_active]));

    SetDispMask(1);

    DrawOTag(ctx.ot[ctx.db_active]+OTLEN-1);

    ctx.db_active = !(ctx.db_active);
    ctx.nextpri = ctx.primbuff[ctx.db_active];

    ClearOTagR(ctx.ot[ctx.db_active], OTLEN); 

    mainTimer.time++;
}

void init_gfx(void) {
    ResetGraph(0);
    FrameBuffer *db = &(ctx.db);

    SetDefDispEnv(&(db->disp[0]), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDispEnv(&(db->disp[1]), 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);

    SetDefDrawEnv(&(db->draw[0]), 0, SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT);
    SetDefDrawEnv(&(db->draw[1]), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    setRGB0(&(db->draw[0]), 236, 13, 102);
    setRGB0(&(db->draw[1]), 236, 13, 102);

    db->draw[0].isbg = 1;
    db->draw[1].isbg = 1;

    ctx.db_active = 0;

    PutDispEnv(&(db->disp[0]));
    PutDrawEnv(&(db->draw[0]));

    #if DEBUG_MODE
        init_debug_fnt();
    #endif

    ctx.nextpri = ctx.primbuff[ctx.db_active];

    ClearOTagR(ctx.ot[ctx.db_active], OTLEN); 

    create_timer(&(mainTimer));
}

void init_debug_fnt(void) {
    FntLoad(960, 0);
    fnt = FntOpen(0, 8, 320, 224, 0, 100);
}