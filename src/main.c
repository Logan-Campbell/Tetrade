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

/**
 * All the game logic is contained in this file.
 * 
 * 
 * TODO:
 *  - Tetriminos sometimes drop immediately when spawned (spawns on the edge on the drop timer)
*/


#include "engine/fpmath.h"
#include "engine/graphics2d.h"
#include "engine/input.h"
#include "engine/text.h"
#include "engine/audio.h"

#define MATRIX_WIDTH 10
#define MATRIX_HEIGHT 20
#define MINO_WIDTH 8
#define MINO_SMALL_WIDTH 4
#define NUM_NEXT_TETRIMINOS 14
#define NUM_TETRIMINO_TYPES 7
#define NUM_TETRIMINO_EXTRAS 1
#define TETRIMINO_HORZ_SPEED 3
#define TETRIMINO_MOVE_COOLDOWN 10
#define TETRIMINO_SET_TIME 80
#define CENTER 3

#define BASE_DROP_RATE 30
#define LEVEL_DROP_RATE_MULTI 4506 //1.10, Fixed int
#define SOFT_DROP_RATE 4
#define MAIN_MENU_OPTIONS 3
#define OPTIONS_MENU_OPTIONS 3
#define CONTINUE_TIME (10 * VYSNC_RATE)
#define PAUSE_TIME (3 * VYSNC_RATE)
#define MUSIC_CHANNEL 0 

#define SINGLE_LINE_SCORE 200
#define DOUBLE_LINE_SCORE 500
#define TRIPLE_LINE_SCORE 700
#define TETRA_LINE_SCORE  1000
#define SOFT_DROP_SCORE   1
#define HARD_DROP_SCORE   2
#define LEVEL_GOAL        8

extern const uint8_t click[];
extern const uint8_t confirm[];
extern const uint8_t place[];
extern const uint8_t clear[];
extern const uint8_t theme[];
extern const uint8_t negative[];
extern const uint8_t hold[];

typedef enum _GameState {
    START   = 0,
    REGULAR = 1,
    VERSUS  = 2
} GameState;

typedef enum _MenuState {
    PRESS_START = 0,
    MAIN_MENU = 1,
    OPTIONS = 2
} MenuState;

typedef struct _Game {
    GameState gameState;
    MenuState menuState;
    TextSprite scoreText;
    TextSprite bigText;
    Sprite title;
    Sprite backgroundLeft;
    Sprite backgroundRight;
    Sprite foregroundLeft;
    Sprite foregroundRight;
    Sprite tetriminoSprites[NUM_TETRIMINO_TYPES+NUM_TETRIMINO_EXTRAS];
    Sprite tetriminoGhostSprites[NUM_TETRIMINO_TYPES+NUM_TETRIMINO_EXTRAS];
    Sprite tetriminoSmallSprites[NUM_TETRIMINO_TYPES+NUM_TETRIMINO_EXTRAS];

    AudioSample click_sfx;
    AudioSample confirm_sfx;
    AudioSample place_sfx;
    AudioSample clear_sfx;
    AudioSample negative_sfx;
    AudioSample hold_sfx;
    AudioSample theme_song;

    int isMusicPlaying;

    // Settings
    int isRandomBag;
    int sfxVol;
    int musicVol;

    int playerOneStart;
    int playerTwoStart;

    int winner;

    int selectedOption;
    Timer mainTimer;
} Game;


typedef struct _Tetrimino {
    int type;
    int shape[4][4]; // Reletive position of the minos
    int x, y;        // Postion on matrix (top left point of shape matrix)
    int wasHeld;
    int rotState;

    Sprite *tetrimnoSprite;
    Sprite *tetrimnoSmallSprite;
} Tetrimino;

typedef struct _TetradeGame {
    int matrix[MATRIX_HEIGHT][MATRIX_WIDTH];
    Tetrimino tetrimino;
    int nextTCount;
    Tetrimino nextTetriminos[NUM_NEXT_TETRIMINOS];
    Tetrimino holdTerimino;

    int score;
    int singleLine;
    int doubleLine;
    int tripleLine;
    int tetrade;

    int moveCooldown;
    int setTime;
    int level;
    int ghostY;
    int isGameOver;
    int isGamePaused;
    int m_u, m_v; // For traversing the matrix whem game is lost

    // X,Y postions for graphics
    int matrixX, matrixY;
    int scoreX, scoreY;
    int singleX, singleY;
    int doubleX, doubleY;
    int tripleX, tripleY;
    int tetrisX, tetrisY;
    int levelX, levelY;
    int continueX, continueY;
    int continueCountX, continueCountY;
    int nextXY[3][2];
    int holdX, holdY;
    
    struct _TetradeGame *opponent;
    int controller;
    Timer pauseTimer;
    Timer continueTimer;
    Timer gameTimer;
} TetradeGame;

static Game gameCtx;

static const int pieces[7][4][4] = {
        {{0, 0, 0, 0}, 
         {1, 1, 1, 1}, 
         {0, 0, 0, 0}, 
         {0, 0, 0, 0}}, //I piece

        {{0, 0, 0, 0}, 
         {0, 1, 1, 0}, 
         {0, 1, 1, 0}, 
         {0, 0, 0, 0}}, //O piece

        {{1, 0, 0, 0}, 
         {1, 1, 1, 0}, 
         {0, 0, 0, 0}, 
         {0, 0, 0, 0}}, //J piece

        {{0, 0, 1, 0}, 
         {1, 1, 1, 0}, 
         {0, 0, 0, 0}, 
         {0, 0, 0, 0}}, //L piece

        {{1, 1, 0, 0}, 
         {0, 1, 1, 0}, 
         {0, 0, 0, 0}, 
         {0, 0, 0, 0}}, //Z piece

        {{0, 1, 1, 0}, 
         {1, 1, 0, 0}, 
         {0, 0, 0, 0}, 
         {0, 0, 0, 0}}, //S piece

        {{0, 1, 0, 0}, 
         {1, 1, 1, 0}, 
         {0, 0, 0, 0}, 
         {0, 0, 0, 0}}, //T piece
    };

// Tables below are modified version of Tetris SRS, for more information: https://tetris.fandom.com/wiki/SRS

// X,Y postions to check when rotations are obstructed, adds a "Wall/Floor Kick" ability when rotating 
// 0 = spawn state 1 = rotated clockwise sate 2 = twice rotated sate 3 = rotated counter clockwise state
static const int wallKickNormalTests[8][5][2] = {
        {{-1,  0}, {-1,  1}, { 0, -2}, {-1, -2}, { 0,  0}}, //0>>1
        {{ 1,  0}, { 0,  1}, { 1, -1}, { 0,  2}, { 1,  2}}, //1>>0
        {{ 1,  0}, { 0,  1}, { 1, -1}, { 0,  2}, { 1,  2}}, //1>>2
        {{-1,  0}, {-1,  1}, { 0, -2}, {-1, -2}, { 0,  0}}, //2>>1
        {{ 1,  0}, { 0,  1}, { 1,  1}, { 0, -2}, { 1, -2}}, //2>>3
        {{-1,  0}, {-1, -1}, { 0,  2}, {-1,  2}, { 1,  0}}, //3>>2
        {{-1,  0}, { 0,  1}, {-1, -1}, { 0,  2}, {-1,  2}}, //3>>0
        {{ 1,  0}, { 1,  1}, { 0, -2}, { 1, -2}, { 0,  0}}, //0>>3
    };

// Normal tests don't not work with the I piece, so special test set is required.
static const int wallKickITests[8][5][2] = {
        {{-2,  0}, { 1,  0}, {-2, -1}, { 1,  2}, { 0, -2}}, //0>>1
        {{ 2,  0}, {-1,  0}, { 2,  1}, {-1, -2}, { 0,  0}}, //1>>0
        {{-1,  0}, { 2,  0}, {-1,  2}, { 2, -1}, { 0,  0}}, //1>>2
        {{ 1,  0}, {-2,  0}, { 1, -2}, {-2,  1}, { 0, -2}}, //2>>1
        {{ 2,  0}, {-1,  0}, { 2,  1}, {-1, -2}, { 0, -2}}, //2>>3
        {{-2,  0}, { 1,  0}, {-2, -1}, { 1,  2}, { 0,  0}}, //3>>2
        {{ 1,  0}, {-2,  0}, { 1, -2}, {-2,  1}, { 0,  0}}, //3>>0
        {{-1,  0}, { 2,  0}, {-1,  2}, { 2, -1}, { 0, -2}}, //0>>3
    };

static const int musicSRsbyLevel[10] = { 22050, 23152, 24310, 25525, 26802, 28142, 29546, 31026, 32577, 34206 };
static const int volumeLevels[11] = { 0x0000, 0x0666, 0x0CCC, 0x1332, 0x1999, 0x1FFF, 0x2665, 0x2CCC, 0x3332, 0x3998, 0x3fff };

// 0 counterClockwise 1 clockwise
int get_rot_state(const int direction, const int rotState) {
    int rs = rotState;
    if(direction) rs++;
    else          rs--;

    if(rs > 3) rs = 0;
    if(rs < 0) rs = 3;

    return rs;
}

int get_rot_test(const int direction, const int rotState) {
    switch(rotState) {
        case 0:
            if(direction)
                return 0;
            else    
                return 7;
        case 1:
            if(direction)
                return 2;
            else    
                return 1;
        case 2:
             if(direction)
                return 4;
            else    
                return 3;
        case 3:
             if(direction)
                return 6;
            else    
                return 5;
        default:
            return 0;
    }
}

//Sets the tetrimino to hav ethe correct data based on type
void pick_tetrimino(Tetrimino *tetrimino, const int type) {
    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++) {
            tetrimino->shape[row][col] = pieces[type-1][row][col];
        }
    }

    tetrimino->type = type;

    //Even width pieces spawn dead center odd width pieces should spawn center left
    tetrimino->x = CENTER;

    //Tetriminos should spawn with the top most mino(s) touching the top
    tetrimino->y = (type > 1) ? 0 : -1;

    tetrimino->wasHeld = 0;
    tetrimino->rotState = 0;
}

void set_random_tetrimino(Tetrimino *tetrimino) {
        pick_tetrimino(tetrimino, ((rand() % 7) + 1));
}

// Return true if all minos are within bounds and do not overlap other minos
int is_valid_move(const int x, const int y, const Tetrimino *tetrimino, 
                const int matrix[MATRIX_HEIGHT][MATRIX_WIDTH]) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            if(tetrimino->shape[i][j] > 0) {
                if(x + j < 0 || x + j >= MATRIX_WIDTH || 
                    y + i < 0 || y + i >= MATRIX_HEIGHT || 
                    matrix[y+i][x+j] > 0) {
                    return 0;
                }
            }
        }
    }

    return 1;
}

// Add minos to the matrix
void place_tetrimino(Tetrimino *tetrimino, int matrix[MATRIX_HEIGHT][MATRIX_WIDTH]) {
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            if(tetrimino->shape[i][j] > 0) {
                matrix[tetrimino->y+i][tetrimino->x+j] = tetrimino->type;
            }
        }
    }
}

// Remove bottom row of the matrix and push everything down
void remove_row(const int row, const int rowc, const int colc, int matrix[rowc][colc]) {
    for(int i = row; i > 0; i-- ) {
        for(int j = 0; j < colc; j++) {
            matrix[i][j] = matrix[i-1][j];
        }
    }

    // Make sure top row is empty
    for(int i = 0; i < colc; i++) {
        matrix[0][i] = 0;
    }
}

// Add row of minos to the bottom of the matrix with one mino missing
void add_garbage(int matrix[MATRIX_HEIGHT][MATRIX_WIDTH]) {
    for(int i = 0; i < MATRIX_HEIGHT-1; i++ ) {
        for(int j = 0; j < MATRIX_WIDTH; j++) {
            matrix[i][j] = matrix[i+1][j];
        }
    }

    for(int i = 0; i < MATRIX_WIDTH; i++) {
        matrix[MATRIX_HEIGHT-1][i] = 8;
    }
    matrix[MATRIX_HEIGHT-1][rand() % 8] = 0;
}

void set_music_speed_by_level(TetradeGame *game) {
    int highestLevel = (game->opponent == NULL || game->level >= game->opponent->level ) ? game->level : game->opponent->level;
    change_ch_sample_rate(0, musicSRsbyLevel[highestLevel]);
}

// Check for complete rows, remove rows, add garbage, increase level, and score points.
void check_lines(TetradeGame *game) {
    int linesCleared = 0;
    for(int row = 0; row < MATRIX_HEIGHT; row++) {
        int isFullRow = 1;
        for(int col = 0; col < MATRIX_WIDTH; col++) {
            if(game->matrix[row][col] <= 0) {
                isFullRow = 0;
                break;
            }
        }

        if(isFullRow) {
            remove_row(row, MATRIX_HEIGHT, MATRIX_WIDTH, game->matrix);
            //Adding garbage to a game overed opponent looks weird
            if(game->opponent != NULL && !game->opponent->isGameOver) {
                add_garbage(game->opponent->matrix);
            }
            
            linesCleared++;
        }
    }
    
    if(linesCleared > 0) {
        play_sample(&(gameCtx.clear_sfx)); //Audio played here so not to stack
        int totalLines = game->singleLine + game->doubleLine*2 + game->tripleLine*3 + game->tetrade*4;

        if(totalLines >= (LEVEL_GOAL * (game->level+1))) {
            game->level++;

            set_music_speed_by_level(game);
        }
    }

    switch(linesCleared) {
        case 1:
            game->singleLine++;
            game->score += SINGLE_LINE_SCORE * game->level;
            break;
        case 2:
            game->doubleLine++;
            game->score += DOUBLE_LINE_SCORE * game->level;
            break;
        case 3:
            game->tripleLine++;
            game->score += TRIPLE_LINE_SCORE * game->level;
            break;
        case 4:
            game->tetrade++;
            game->score += TETRA_LINE_SCORE * game->level;
            break;
        default:
            break;
    }
}

// Draws a line of white tiles
void draw_squares(const int x, const int y, const int w, const int h, int spacing, const int num) {
    int startX = x;
    for(int i = 0; i < num; i++) {
        draw_tile(
            (CVECTOR) {.r = 255, .g = 255, .b = 255},
            startX,
            y,
            w,
            h
        );
        startX += w+spacing;
    }
}

void draw_tetrimino(const int x, const int y, const int minoWidth, 
                    Tetrimino *tetrimino, Sprite *tetriminoSprites, const int isOffset) {
    
    int offsetX = 0;
    int offsetY = 0;
    if(isOffset && tetrimino->type > 2) {
        offsetX += minoWidth/2;
        offsetY += minoWidth;
    }

    int startX = x + offsetX;
    int startY = y + offsetY;

    Sprite *sprite;

    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 4; j++) {
            if(tetrimino->shape[i][j] > 0) {
                    sprite = &tetriminoSprites[tetrimino->type-1];
                    sprite->x = startX + minoWidth/2;
                    sprite->y = startY + minoWidth/2;
                    draw_sprite(sprite);

            }
            startX += minoWidth;
        }
        
        startY += minoWidth;
        startX = x + offsetX;
    }
}

void draw_matrix(const int x, const int y, TetradeGame *game) {
    int startX = x;
    int startY = y;
    Sprite *sprite;
    for(int row = 0; row < MATRIX_HEIGHT; row++) {
        for(int col = 0; col < MATRIX_WIDTH; col++) {

            //Draw minos on matrix
            if(game->matrix[row][col] > 0) {

                sprite = &(gameCtx.tetriminoSprites[game->matrix[row][col]-1]);
                sprite->x = startX + MINO_WIDTH/2;
                sprite->y = startY + MINO_WIDTH/2;
                draw_sprite(sprite);
            }
            
            startX += MINO_WIDTH;
        }
        startY += MINO_WIDTH;
        startX = x;
    }

    if(game->tetrimino.type > 0) {
        //Current tetrimino
        draw_tetrimino(x + (game->tetrimino.x * MINO_WIDTH), 
                      y + (game->tetrimino.y * MINO_WIDTH), 
                      MINO_WIDTH, 
                      &(game->tetrimino), 
                      gameCtx.tetriminoSprites,
                      0);
        
        //Ghost tetrimino
        draw_tetrimino(x + (game->tetrimino.x * MINO_WIDTH), 
                      y + (game->ghostY * MINO_WIDTH), 
                      MINO_WIDTH, 
                      &(game->tetrimino), 
                      gameCtx.tetriminoGhostSprites,
                      0);
    }

    print_text(&(gameCtx.scoreText), game->scoreX,  game->scoreY,  "%6d", game->score);
    print_text(&(gameCtx.scoreText), game->singleX, game->singleY, "%6d", game->singleLine);
    print_text(&(gameCtx.scoreText), game->doubleX, game->doubleY, "%6d", game->doubleLine);
    print_text(&(gameCtx.scoreText), game->tripleX, game->tripleY, "%6d", game->tripleLine);
    print_text(&(gameCtx.scoreText), game->tetrisX, game->tetrisY, "%6d", game->tetrade);
    print_text(&(gameCtx.scoreText), game->levelX,  game->levelY,  "%6d", game->level);


    draw_tetrimino(game->nextXY[0][0], game->nextXY[0][1], MINO_WIDTH, &(game->nextTetriminos[0]), gameCtx.tetriminoSprites, 1);
    draw_tetrimino(game->nextXY[1][0], game->nextXY[1][1], MINO_SMALL_WIDTH, &(game->nextTetriminos[1]), gameCtx.tetriminoSmallSprites, 1);
    draw_tetrimino(game->nextXY[2][0], game->nextXY[2][1], MINO_SMALL_WIDTH, &(game->nextTetriminos[2]), gameCtx.tetriminoSmallSprites, 1);

    if(game->holdTerimino.type > 0) {
        draw_tetrimino(game->holdX, game->holdY, MINO_SMALL_WIDTH, &(game->holdTerimino), gameCtx.tetriminoSmallSprites, 1);
    }
}

void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// start: inclusive
// end: exclusive
// length: length of the array
void shuffle_elements(const int start, const int end, const int length, int arr[length]) {
    //Fisher-Yates shuffle
    for(int i = end-1; i > start; i--) {
        int r = rand() % (i + 1) + start;
        swap(&arr[i], &arr[r]);
    }
}

void reverse_columns(const int h, const int w, const int rowc, 
                        const int colc, int arr[rowc][colc]) {
    for (int i = 0; i < h; i++) {
        for (int j = 0, k = w - 1; j < k; j++, k--) {
            swap(&arr[j][i], &arr[k][i]);
        }
    }         
}
 
void reverse_rows(const int h, const int w, const int rowc, 
                    const int colc, int arr[rowc][colc]) {
    for (int i = 0; i < h; i++) {
        for (int j = 0, k = w - 1; j < k; j++, k--) {
            swap(&arr[i][j], &arr[i][k]);
            
        }
    }         
}

void transpose(const int h, const int w, const int rowc, 
                    const int colc, int arr[rowc][colc]) {
    for (int i = 0; i < h; i++) {
        for (int j = i; j < w; j++) {
            swap(&arr[i][j], &arr[j][i]);
        }
    }
}

void rotate_clockwise(const int h, const int w, const int rowc, 
                        const int colc, int matrix[rowc][colc]) {
    transpose(h, w, rowc, colc, matrix);
    reverse_rows(h, w, rowc, colc, matrix);
}

void rotate_counterclockwise(const int h, const int w, const int rowc, 
                            const int colc, int matrix[rowc][colc]) {
    transpose(h, w, rowc, colc, matrix);
    reverse_columns(h, w, rowc, colc, matrix);
}

// 0/false = counter clockwise  1/true clockwise
void rotate_tetrimino(const int direction, TetradeGame *game) {
    // Make a copy so that rotation can be applied to and checked for validity
    Tetrimino copy = game->tetrimino;

    // Reduce the shape size to get proper rotation centers for non I and O tetrominoes
    int tetSize = (copy.type > 2) ? 3 : 4;

    if(direction) rotate_clockwise(tetSize, tetSize, 4, 4, copy.shape);
    else          rotate_counterclockwise(tetSize, tetSize, 4, 4, copy.shape);

    if(!is_valid_move(copy.x, copy.y, &copy, game->matrix)) {
        int xy[2];
        int test = get_rot_test(direction, copy.rotState);

        for(int i = 0; i < 5; i++){
            if(copy.type == 1) {
                xy[0] = wallKickITests[test][i][0];
                xy[1] = wallKickITests[test][i][1];
            } else {
                xy[0] = wallKickNormalTests[test][i][0];
                xy[1] = wallKickNormalTests[test][i][1];
            }
            

            if(is_valid_move(copy.x + xy[0], copy.y + xy[1], &copy, game->matrix)) {
                copy.x += xy[0];
                copy.y += xy[1];
                copy.rotState = get_rot_state(direction, copy.rotState);
                game->tetrimino = copy;
                break;
            }
        }
    } else {
        game->tetrimino = copy;
        game->tetrimino.rotState = get_rot_state(direction, game->tetrimino.rotState);
    }

}

// Load Assets, intialize variables
void init_game(Game *game) {

    //Load Text
    TIM_IMAGE textSheetImage;
    extern uint32_t text_sheet_image[];
    load_texture(text_sheet_image, &textSheetImage);

    int charNum = 95;
    game->scoreText.spritesList = malloc(sizeof(Sprite)*charNum);
    char charList[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    load_text(&(game->scoreText), charList, &textSheetImage, 8, 8, charNum);

    //Load title
    TIM_IMAGE titleImage;
    extern uint32_t title[];
    load_texture(title, &titleImage);
    load_sprite(&(game->title), &titleImage);

    //Load Left Background
    TIM_IMAGE backgroundLeftImage;
    extern uint32_t background_left[];
    load_texture(background_left, &backgroundLeftImage);
    load_sprite(&(game->backgroundLeft), &backgroundLeftImage);

    //Load Right Background
    TIM_IMAGE backgroundRightImage;
    extern uint32_t background_right[];
    load_texture(background_right, &backgroundRightImage);
    load_sprite(&(game->backgroundRight), &backgroundRightImage);

    //Load Left Foreground
    TIM_IMAGE foregroundLeftImage;
    extern uint32_t foreground_left[];
    load_texture(foreground_left, &foregroundLeftImage);
    load_sprite(&(game->foregroundLeft), &foregroundLeftImage);

    //Load Right Foreground
    TIM_IMAGE foregroundRightImage;
    extern uint32_t foreground_right[];
    load_texture(foreground_right, &foregroundRightImage);
    load_sprite(&(game->foregroundRight), &foregroundRightImage);

    //Load Big Text
    TIM_IMAGE bigFontImage;
    extern uint32_t big_font[];
    load_texture(big_font, &bigFontImage);

    charNum = 39;
    game->bigText.spritesList = malloc(sizeof(Sprite)*charNum);
    char charList2[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!?";
    load_text(&(game->bigText), charList2, &bigFontImage, 16, 16, charNum);

    //Set background positions
    game->title.x = (SCREEN_WIDTH-game->title.w)/2;
    game->title.y = 80;

    game->backgroundLeft.x = 0;  //game->backgroundLeft.x = 80;
    game->backgroundLeft.y = 0; //game->backgroundLeft.y = 120;

    game->backgroundRight.x = 160; //game->backgroundRight.x = 240;
    game->backgroundRight.y = 0; //game->backgroundRight.y = 120;

    game->foregroundLeft.x = 0;   //game->foregroundLeft.x = 80;
    game->foregroundLeft.y = 0;  //game->foregroundLeft.y = 120;

    game->foregroundRight.x = 160; //game->foregroundRight.x = 240;
    game->foregroundRight.y = 0; //game->foregroundRight.y = 120;


    //Load Mino Sprites
    TIM_IMAGE minosImage;
    extern uint32_t blocks[];
    load_texture(blocks, &minosImage);

    TIM_IMAGE minosSmallImage;
    extern uint32_t blocks_small[];
    load_texture(blocks_small, &minosSmallImage);

    int numMinos = NUM_TETRIMINO_TYPES + NUM_TETRIMINO_EXTRAS;
    Sprite minos[numMinos*2];
    Sprite minosSmall[numMinos];
    load_sprite_sheet(minos, MINO_WIDTH, MINO_WIDTH, numMinos*2, numMinos, &minosImage);
    load_sprite_sheet(minosSmall, 4, 4, numMinos, numMinos, &minosSmallImage);

    for(int i = 0; i < numMinos; i++) {
        game->tetriminoSprites[i] = minos[i];
        game->tetriminoGhostSprites[i] = minos[numMinos + i];
        game->tetriminoSmallSprites[i] = minosSmall[i];
    }

    game->isMusicPlaying = 0;
    game->isRandomBag = 1;
    game->sfxVol = 11;
    game->musicVol = 5;
    game->playerOneStart = 0;
    game->playerTwoStart = 0;
    game->winner = -1;

    //Audio
    init_sample_byte(&(game->click_sfx), click);
    init_sample_byte(&(game->confirm_sfx), confirm);
    init_sample_byte(&(game->place_sfx), place);
    init_sample_byte(&(game->clear_sfx), clear);
    init_sample_byte(&(game->negative_sfx), negative);
    init_sample_byte(&(game->hold_sfx), hold);
    init_sample_byte(&(game->theme_song), theme);
    

    game->click_sfx.volume    = volumeLevels[game->sfxVol];
    game->confirm_sfx.volume  = volumeLevels[game->sfxVol];

    game->place_sfx.volume    = volumeLevels[game->sfxVol];
    game->clear_sfx.volume    = volumeLevels[game->sfxVol];
    
    game->negative_sfx.volume = volumeLevels[game->sfxVol];
    game->hold_sfx.volume     = volumeLevels[game->sfxVol];
    game->theme_song.volume   = volumeLevels[game->musicVol];

    create_timer(&(game->mainTimer));
    game->gameState = START;
    game->menuState = PRESS_START;

}

void reset_tetris_game(TetradeGame *game) {
    for(int row = 0; row < MATRIX_HEIGHT; row++) {
        for(int col = 0; col < MATRIX_WIDTH; col++) {
            game->matrix[row][col] = 0;
        }
    }

    game->score = 0;
    game->singleLine = 0;
    game->doubleLine = 0;
    game->tripleLine = 0;
    game->tetrade = 0;
    game->level = 0;
    game->ghostY = 0;
    game->isGameOver = 0;
    game->isGamePaused = 0;
    game->m_u = MATRIX_HEIGHT-1;
    game->m_v = 0;

    game->holdTerimino.type = -1;
    game->tetrimino.type = -1;

    
    if(gameCtx.isRandomBag) {
        for(int i = 0; i < NUM_NEXT_TETRIMINOS; i += NUM_TETRIMINO_TYPES){
            int typeArr[] = {1, 2, 3, 4, 5, 6, 7 };
            shuffle_elements(0, 7, 7, typeArr);
            for(int k = 0; k < 7; k++) {
                pick_tetrimino(&(game->nextTetriminos[i+k]), typeArr[k]);
            } 
        }
    } else {
        for(int i = 0; i < NUM_NEXT_TETRIMINOS; i++) {
            set_random_tetrimino(&(game->nextTetriminos[i]));
        }
    }

    game->nextTCount = NUM_NEXT_TETRIMINOS;
    
    create_timer(&(game->gameTimer));
    create_timer(&(game->continueTimer));
    create_timer(&(game->pauseTimer));
    game->continueTimer.time = CONTINUE_TIME;
    game->pauseTimer.time = PAUSE_TIME;
    game->moveCooldown = 0;




}

// Set TetradeGame values.
void init_tetris_game(TetradeGame *game, const int controller) {
    if(controller) {
        game->matrixX = 207;
        game->matrixY = 36;
        game->scoreX = 158;
        game->scoreY = 69;
        game->singleX = 158;
        game->singleY = 93;
        game->doubleX = 158;
        game->doubleY = 117;
        game->tripleX = 158;
        game->tripleY = 141;
        game->tetrisX = 158;
        game->tetrisY = 165;
        game->levelX = 158;
        game->levelY = 189;
        game->continueX = 171;
        game->continueY = 42;
        game->continueCountX = 228;
        game->continueCountY = 62;

        game->nextXY[0][0] = 170;
        game->nextXY[0][1] = 16;
        game->nextXY[1][0] = 210;
        game->nextXY[1][1] = 18;
        game->nextXY[2][0] = 232;
        game->nextXY[2][1] = 18;

        game->holdX = 294;
        game->holdY = 18;
    } else {
        game->matrixX = 25;  //29
        game->matrixY = 36; //40
        game->scoreX = 108; //112
        game->scoreY = 69;  //73
        game->singleX = 108;
        game->singleY = 93;
        game->doubleX = 108;
        game->doubleY = 117;
        game->tripleX = 108;
        game->tripleY = 141;
        game->tetrisX = 108;
        game->tetrisY = 165;
        game->levelX = 108;
        game->levelY = 189;
        game->continueX = 6;
        game->continueY = 42;
        game->continueCountX = 46;
        game->continueCountY = 62;

        game->nextXY[0][0] = 110;
        game->nextXY[0][1] = 16;
        game->nextXY[1][0] = 90;
        game->nextXY[1][1] = 18;
        game->nextXY[2][0] = 68;
        game->nextXY[2][1] = 18;

        game->holdX = 7;
        game->holdY = 18;
    }

    game->opponent = NULL;
    game->controller = controller;

    reset_tetris_game(game);
}

int last_valid_y(const TetradeGame *game) {
    int y = game->tetrimino.y;
    while(is_valid_move(game->tetrimino.x, y+1, &(game->tetrimino), game->matrix) && y <= MATRIX_HEIGHT) {
        y++;
    }
    return y;
}

// return 0 - reset game
// return 1 - continue game
int lose_game(TetradeGame *game) {

    //Turn minos placed on the matrix into "lost" minos two at time for speed
    if(game->m_u >= 0 && game->m_v < MATRIX_WIDTH) {
        if(game->matrix[game->m_u][game->m_v] > 0) {
            game->matrix[game->m_u][game->m_v] = 8;
        }

        if(game->matrix[game->m_u][game->m_v+1] > 0) {
            game->matrix[game->m_u][game->m_v+1] = 8;
        }

        game->m_v += 2;
        if(game->m_v >= MATRIX_WIDTH-1) {
            game->m_v = 0;
            game->m_u--;
        }
    }
    
    //Only after all minos are changed show continue countdown
    if(game->gameTimer.time > 120) {
        //In VERSUS mode, there are no continues.
        if(game->opponent != NULL) {
            return 0;
        }

        print_text(&(gameCtx.bigText), game->continueX, game->continueY, "CONTINUE?");
        print_text(&(gameCtx.bigText), game->continueCountX, game->continueCountY, "%2d", TimerSeconds(&(game->continueTimer)));

        if(button_down(game->controller, PAD_START)) {
            reset_tetris_game(game);
            game->isGameOver = 0;
            set_music_speed_by_level(game);
        }

        //Do not continue, just lose
        if(button_down(game->controller, PAD_CIRCLE)) {
            return 0;
        }

        if(game->continueTimer.time < 1) {
            return 0;
        } else {
            game->continueTimer.time--;
        }
    }
    
    game->gameTimer.time++;
    return 1;
}

int play_game(TetradeGame *game) {
 
    //Stall game if in game over state
    if(game->isGameOver) {
        int isContinue = lose_game(game);
        //Draw matrix after so lose game continue text appears on top
        draw_matrix(game->matrixX, game->matrixY, game);
        return isContinue;
    }
    int controller = game->controller;

    //Pause game. Should not pause when in VERSUS mode
    if(!game->isGamePaused && game->opponent == NULL && button_down(controller, PAD_START)) {
        game->isGamePaused = 1;

    } else if(game->isGamePaused) {
        print_text(&(gameCtx.bigText), game->continueX+16, game->continueY, "PAUSED");
        draw_matrix(game->matrixX, game->matrixY, game);

        if(button_down(controller, PAD_START)) {
            game->isGamePaused = 0;
            game->pauseTimer.time = PAUSE_TIME;
        }

        return 1;
    }

    //Countdown timer when game is unpaused
    if(game->pauseTimer.time > 1) {
        print_text(&(gameCtx.bigText), game->continueCountX, game->continueCountY, "%2d", TimerSeconds(&(game->pauseTimer)));
        game->pauseTimer.time--;
        draw_matrix(game->matrixX, game->matrixY, game);
        return 1;
    }
    
    int isSoftDropping = 0;

    // Move Piece Left
    if(button_down(controller, PAD_LEFT)) {
        if(is_valid_move(game->tetrimino.x-1, game->tetrimino.y, &(game->tetrimino), game->matrix)) {
            game->tetrimino.x--;
            game->moveCooldown = TETRIMINO_MOVE_COOLDOWN;
            play_sample(&(gameCtx.click_sfx));
        }
    // Move Piece Left continuously
    } else if(button_pressed(controller, PAD_LEFT)) {
        if(is_valid_move(game->tetrimino.x-1, game->tetrimino.y, &(game->tetrimino), game->matrix) &&
            (game->gameTimer.time%TETRIMINO_HORZ_SPEED == 0) && game->moveCooldown <= 0) {
            game->tetrimino.x--;
            play_sample(&(gameCtx.click_sfx));
        }
    // Move Piece Right
    } else if(button_down(controller, PAD_RIGHT)) {
        if(is_valid_move(game->tetrimino.x+1, game->tetrimino.y, &(game->tetrimino), game->matrix)) {
            game->tetrimino.x++;
            game->moveCooldown = TETRIMINO_MOVE_COOLDOWN;
            play_sample(&(gameCtx.click_sfx));
        }
    // Move Piece Right continuously
    } else if(button_pressed(controller, PAD_RIGHT)) {
        if(is_valid_move(game->tetrimino.x+1, game->tetrimino.y, &(game->tetrimino), game->matrix) &&
            (game->gameTimer.time%TETRIMINO_HORZ_SPEED == 0) && game->moveCooldown <= 0) {
            game->tetrimino.x++;
            play_sample(&(gameCtx.click_sfx));
        }
    } 
    
    // Soft Drop
    if(button_pressed(controller, PAD_DOWN)) {
        isSoftDropping = 1;
    }
    
    // Rotate Clockwise
    if(button_down(controller, PAD_CIRCLE)) {
        rotate_tetrimino(1, game);
    // Rotate Counterclockwise
    } else if (button_down(controller, PAD_CROSS)) {
        rotate_tetrimino(0, game);
    }

    // Hard Drop
    if(button_down(controller, PAD_UP)) {
        int y = last_valid_y(game);
        game->score += (y - game->tetrimino.y) * HARD_DROP_SCORE;
        game->tetrimino.y = y;
        place_tetrimino(&(game->tetrimino), game->matrix);
        game->tetrimino.type = -1;
        play_sample(&(gameCtx.place_sfx));

    // Hold
    }else if(button_down(controller, PAD_SQUARE) || 
             button_down(controller, PAD_L1)     ||
             button_down(controller, PAD_R1)) {
        if((game->tetrimino.type > 0) && !(game->tetrimino.wasHeld)) {
            Tetrimino temp = game->holdTerimino;
            game->holdTerimino = game->tetrimino;
            // Resets tetrimino when placed in hold
            pick_tetrimino(&(game->holdTerimino), game->holdTerimino.type);
            game->tetrimino = temp;
            
            game->tetrimino.wasHeld = 1;
            game->tetrimino.x = CENTER;
            game->tetrimino.y = 0;

            play_sample(&(gameCtx.hold_sfx));
        } else if(game->tetrimino.wasHeld) {
            play_sample(&(gameCtx.negative_sfx));
        }
    }

    // Set next tetrimino to current tetrimino, if there is no current tetrimino.
    if(game->tetrimino.type <= 0) {
        game->tetrimino = game->nextTetriminos[0];
        for(int i = 0; i < NUM_NEXT_TETRIMINOS-1; i++) {
            game->nextTetriminos[i] = game->nextTetriminos[i+1];
        }

        game->nextTCount--;

        // Replace removed tetriminos
        if(gameCtx.isRandomBag) {
            if(game->nextTCount <= NUM_TETRIMINO_TYPES) {

                int typeArr[] = {1, 2, 3, 4, 5, 6, 7};
                shuffle_elements(0, 7, 7, typeArr);
                for(int i = 0; i < 7; i++) {
                    pick_tetrimino(&(game->nextTetriminos[NUM_TETRIMINO_TYPES+i]), typeArr[i]);
                }  

                game->nextTCount = NUM_NEXT_TETRIMINOS;
            }
        } else {
            // Last next tetrimino is empty, replace it.
            set_random_tetrimino(&(game->nextTetriminos[NUM_NEXT_TETRIMINOS-1]));
        }
        
        // If can't place current tetrimino, lose game
        if(!is_valid_move(CENTER, 0, &(game->tetrimino), game->matrix)) {
            place_tetrimino(&(game->tetrimino), game->matrix);
            game->isGameOver = 1;
            game->gameTimer.time = 0;
        }
    }

    int dropRate = SOFT_DROP_RATE;
    if(!isSoftDropping)
        dropRate = (game->level > 0) ? FixedToInt(DivFixed(IntToFixed(BASE_DROP_RATE),(LEVEL_DROP_RATE_MULTI * game->level))) : BASE_DROP_RATE;
    
    // Movement down
    if(game->tetrimino.type > 0 && game->gameTimer.time%dropRate == 0) {
        if(is_valid_move(game->tetrimino.x, game->tetrimino.y+1, &(game->tetrimino), game->matrix)) {
            game->tetrimino.y++;
            game->setTime = TETRIMINO_SET_TIME;
            if(isSoftDropping) {
                game->score += SOFT_DROP_SCORE;
                play_sample(&(gameCtx.click_sfx));
            }
        } else if(game->setTime <= 0) {
            place_tetrimino(&(game->tetrimino), game->matrix);
            play_sample(&(gameCtx.place_sfx));
            game->tetrimino.type = -1;
        }
    }

    game->ghostY = last_valid_y(game);

    if(game->setTime > 0)
        game->setTime--;
    
    check_lines(game);

    if(game->moveCooldown > 0)
        game->moveCooldown--;

    game->gameTimer.time++;

    draw_matrix(game->matrixX, game->matrixY, game);
    return 1;
}

void play_start_menu(void) {
    draw_sprite(&(gameCtx.title));
    if(gameCtx.menuState == PRESS_START) {
        print_text(&(gameCtx.scoreText), 116, 140,  "Press Start");
        if(button_down(0, PAD_START)) {
            gameCtx.menuState = MAIN_MENU;
            play_sample(&(gameCtx.confirm_sfx));
            printf("Seed: %d\n", gameCtx.mainTimer.time);
            srand((unsigned) gameCtx.mainTimer.time);
        }
    } else if(gameCtx.menuState == MAIN_MENU) {
        print_text(&(gameCtx.scoreText), 108, 140,  "Marathon Mode ");
        print_text(&(gameCtx.scoreText), 108, 150,  "Versus Mode ");
        print_text(&(gameCtx.scoreText), 108, 160,  "Options ");
        if(button_down(0, PAD_UP)) {
            gameCtx.selectedOption = (gameCtx.selectedOption - 1 + MAIN_MENU_OPTIONS) % MAIN_MENU_OPTIONS;
            play_sample(&(gameCtx.click_sfx));
        }
        if(button_down(0, PAD_DOWN)) {
            gameCtx.selectedOption = (gameCtx.selectedOption + 1) % MAIN_MENU_OPTIONS;
            play_sample(&(gameCtx.click_sfx));
        }

        switch(gameCtx.selectedOption) {
            case 0:
                print_text(&(gameCtx.scoreText), 100, 140,  ">Marathon Mode<");
                break;
            case 1:
                print_text(&(gameCtx.scoreText), 100, 150,  ">Versus Mode<");
                break;
            case 2:
                print_text(&(gameCtx.scoreText), 100, 160,  ">Options<");
                break;
            default:
                printf("Selection Error. Selection Option: %d\n", gameCtx.selectedOption);
        }

        if(button_down(0, PAD_CROSS) || button_down(0, PAD_START)) {
            play_sample(&(gameCtx.confirm_sfx));
            switch(gameCtx.selectedOption) {
                case 0:
                    gameCtx.gameState = REGULAR;
                    printf("Marathon Mode!\n");
                    break;
                case 1:
                    gameCtx.gameState = VERSUS;
                    printf("Versus Mode!\n");
                    break;
                case 2:
                    gameCtx.menuState = OPTIONS;
                    gameCtx.selectedOption = 0;
                    printf("Options!\n");
                    break;
                default:
                    printf("Selection Error. Selection Option: %d\n", gameCtx.selectedOption);
            }
        }
    } else if(gameCtx.menuState == OPTIONS) {

        print_text(&(gameCtx.scoreText), 40, 140,  
                    "Random Generator: %3s ", ((gameCtx.isRandomBag) ? "Random Bag" : "Pure Random"));
        print_text(&(gameCtx.scoreText), 40, 150, "SFX Volume:   ");
        draw_squares(150, 150, 4, 8, 2, gameCtx.sfxVol);
        print_text(&(gameCtx.scoreText), 40, 160, "Music Volume: ");
        draw_squares(150, 160, 4, 8, 2, gameCtx.musicVol);

        if(button_down(0, PAD_UP)) {
            gameCtx.selectedOption = (gameCtx.selectedOption - 1 + OPTIONS_MENU_OPTIONS) % OPTIONS_MENU_OPTIONS;
            play_sample(&(gameCtx.click_sfx));
        }
        if(button_down(0, PAD_DOWN)) {
            gameCtx.selectedOption = (gameCtx.selectedOption + 1) % OPTIONS_MENU_OPTIONS;
            play_sample(&(gameCtx.click_sfx));
        }

        switch(gameCtx.selectedOption) {
            case 0:
                print_text(&(gameCtx.scoreText), 32, 140,  ">");
                if(button_down(0, PAD_CROSS) || button_down(0, PAD_START)) {
                    play_sample(&(gameCtx.confirm_sfx));
                    gameCtx.isRandomBag = !gameCtx.isRandomBag;
                }
                break;
            case 1:
                print_text(&(gameCtx.scoreText), 32, 150,  ">");
                if(button_down(0, PAD_LEFT) && gameCtx.sfxVol > 0) {
                    gameCtx.sfxVol--;

                    gameCtx.click_sfx.volume    = volumeLevels[gameCtx.sfxVol];
                    gameCtx.confirm_sfx.volume  = volumeLevels[gameCtx.sfxVol];
                    gameCtx.place_sfx.volume    = volumeLevels[gameCtx.sfxVol];
                    gameCtx.clear_sfx.volume    = volumeLevels[gameCtx.sfxVol];
                    gameCtx.negative_sfx.volume = volumeLevels[gameCtx.sfxVol];
                    gameCtx.hold_sfx.volume     = volumeLevels[gameCtx.sfxVol];
                    
                    play_sample(&(gameCtx.confirm_sfx));
                }

                if(button_down(0, PAD_RIGHT) && gameCtx.sfxVol < 11) {
                    gameCtx.sfxVol++;

                    gameCtx.click_sfx.volume    = volumeLevels[gameCtx.sfxVol];
                    gameCtx.confirm_sfx.volume  = volumeLevels[gameCtx.sfxVol];
                    gameCtx.place_sfx.volume    = volumeLevels[gameCtx.sfxVol];
                    gameCtx.clear_sfx.volume    = volumeLevels[gameCtx.sfxVol];
                    gameCtx.negative_sfx.volume = volumeLevels[gameCtx.sfxVol];
                    gameCtx.hold_sfx.volume     = volumeLevels[gameCtx.sfxVol];
                    
                    play_sample(&(gameCtx.confirm_sfx));
                }
                break;
            case 2:
                print_text(&(gameCtx.scoreText), 32, 160,  ">");
                if(button_down(0, PAD_LEFT) && gameCtx.musicVol > 0) {
                    gameCtx.musicVol--;
                    gameCtx.theme_song.volume  = volumeLevels[gameCtx.musicVol];

                    gameCtx.confirm_sfx.volume = volumeLevels[gameCtx.musicVol];
                    play_sample(&(gameCtx.confirm_sfx));
                    gameCtx.confirm_sfx.volume = volumeLevels[gameCtx.sfxVol];
                }

                if(button_down(0, PAD_RIGHT) && gameCtx.musicVol < 11) {
                    gameCtx.musicVol++;
                    gameCtx.theme_song.volume  = volumeLevels[gameCtx.musicVol];

                    gameCtx.confirm_sfx.volume = volumeLevels[gameCtx.musicVol];
                    play_sample(&(gameCtx.confirm_sfx));
                    gameCtx.confirm_sfx.volume = volumeLevels[gameCtx.sfxVol];
                }
                break;
            default:
                printf("Selection Error. Selection Option: %d\n", gameCtx.selectedOption);
        }

        if(button_down(0, PAD_CIRCLE)) {
            gameCtx.menuState = MAIN_MENU;
            gameCtx.selectedOption = 0;
            play_sample(&(gameCtx.confirm_sfx));
        }
    }
} 

void play_regular_mode(TetradeGame *gameOne, TetradeGame *gameTwo) {
    int isContinue1 = 0;
    int isContinue2 = 0;
    
    if(!gameCtx.playerOneStart) {
        play_sample(&(gameCtx.theme_song));
        gameCtx.isMusicPlaying = 1;
        gameCtx.playerOneStart = 1;
        reset_tetris_game(gameOne);
    }

    if(!gameCtx.playerTwoStart && button_down(1, PAD_START)) {            
        gameCtx.playerTwoStart = 1;
        play_sample(&(gameCtx.confirm_sfx));
        reset_tetris_game(gameTwo);
    }

    if(gameCtx.playerOneStart) {
        isContinue1 = play_game(gameOne);
        draw_sprite(&(gameCtx.foregroundLeft));
    }

    if(gameCtx.playerTwoStart) {
        isContinue2 = play_game(gameTwo);
        draw_sprite(&(gameCtx.foregroundRight));
    } else {
        print_text(&(gameCtx.bigText), gameTwo->continueX+32, gameTwo->continueY,    "PRESS");
        print_text(&(gameCtx.bigText), gameTwo->continueX+32, gameTwo->continueY+18, "START");
    }

    if((!isContinue1 && gameCtx.playerOneStart) && 
        ((!isContinue2 && gameCtx.playerTwoStart) || !gameCtx.playerTwoStart)) {

        gameCtx.gameState = START;
        gameCtx.playerOneStart = 0;
        gameCtx.playerTwoStart = 0;
        reset_tetris_game(gameOne);
        reset_tetris_game(gameTwo);
        stop_channel(gameCtx.theme_song.channel);
        gameCtx.isMusicPlaying = 0;
    }
}

void play_versus_mode(TetradeGame *gameOne, TetradeGame *gameTwo) {
    int isContinue1 = 0;
    int isContinue2 = 0;
    
    if(!gameCtx.playerOneStart && button_down(0, PAD_START)) {
        gameCtx.playerOneStart = 1;
        gameOne->opponent = gameTwo;
        play_sample(&(gameCtx.confirm_sfx));
        reset_tetris_game(gameOne);
    }

    if(!gameCtx.playerTwoStart && button_down(1, PAD_START)) {            
        gameCtx.playerTwoStart = 1;
        gameTwo->opponent = gameOne;
        play_sample(&(gameCtx.confirm_sfx));
        reset_tetris_game(gameTwo);
    }

    if(gameCtx.playerOneStart && !gameCtx.playerTwoStart) {
        print_text(&(gameCtx.bigText), gameOne->continueX+16, gameOne->continueY,    "WAITING");
        
    } else if(!gameCtx.playerOneStart) {
        print_text(&(gameCtx.bigText), gameOne->continueX+32, gameOne->continueY,    "PRESS");
        print_text(&(gameCtx.bigText), gameOne->continueX+32, gameOne->continueY+18, "START");
    }

    if(gameCtx.playerTwoStart && !gameCtx.playerOneStart) {
        print_text(&(gameCtx.bigText), gameTwo->continueX+16, gameTwo->continueY,    "WAITING");
        
    } else if(!gameCtx.playerTwoStart) {
        print_text(&(gameCtx.bigText), gameTwo->continueX+32, gameTwo->continueY,    "PRESS");
        print_text(&(gameCtx.bigText), gameTwo->continueX+32, gameTwo->continueY+18, "START");
    }

    if(button_down(0, PAD_CIRCLE) && (!gameCtx.playerOneStart || !gameCtx.playerTwoStart)) {
        gameCtx.gameState = START;
        gameCtx.playerOneStart = 0;
        gameCtx.playerTwoStart = 0;
        play_sample(&(gameCtx.confirm_sfx));
    }   

    if(gameCtx.playerOneStart && gameCtx.playerTwoStart) {
        //Make sure to draw matrix when game is not running.
        if(gameCtx.winner >= 0) {
            draw_matrix(gameOne->matrixX, gameOne->matrixY, gameOne);
            draw_matrix(gameTwo->matrixX, gameTwo->matrixY, gameTwo);
        } else {
            isContinue1 = play_game(gameOne);
            isContinue2 = play_game(gameTwo);
        }
        
        draw_sprite(&(gameCtx.foregroundLeft));
        draw_sprite(&(gameCtx.foregroundRight));

        if(!gameCtx.isMusicPlaying) {
            play_sample(&(gameCtx.theme_song));
            gameCtx.isMusicPlaying = 1;
        }

        if(!isContinue2 && gameCtx.winner < 0)
            gameCtx.winner = 0;

        if(!isContinue1 && gameCtx.winner < 0)
            gameCtx.winner = 1;
        
        if(gameCtx.winner == 0)
            print_text(&(gameCtx.bigText), 52, 208, "PLAYER 1 WINS!");
        
        if(gameCtx.winner == 1)
            print_text(&(gameCtx.bigText), 52, 208, "PLAYER 2 WINS!");
    }

    if((!isContinue1 && gameCtx.playerOneStart) && (!isContinue2 && gameCtx.playerTwoStart)) {
        print_text(&(gameCtx.scoreText), 116, 226, "Press Start");
        if(button_down(0, PAD_START)) {
            gameCtx.gameState = START;
            gameCtx.playerOneStart = 0;
            gameCtx.playerTwoStart = 0;
            reset_tetris_game(gameOne);
            reset_tetris_game(gameTwo);
            gameCtx.winner = -1;
            stop_channel(gameCtx.theme_song.channel);
            gameCtx.isMusicPlaying = 0;
        }
    }
}

int main(void) {
    init_gfx();
    init_input();
    init_audio();
    init_system_timer();

    TetradeGame gameOne;
    TetradeGame gameTwo;

    init_game(&gameCtx);
    init_tetris_game(&gameOne, 0);
    init_tetris_game(&gameTwo, 1);

    DrawSync(0);

    printf("Game Start!\n");

    //Main loop
    while(1) {

        if(gameCtx.gameState == START) {
            play_start_menu();
        } else if(gameCtx.gameState == REGULAR) { 
            play_regular_mode(&gameOne, &gameTwo);
        } else if(gameCtx.gameState == VERSUS) {
            play_versus_mode(&gameOne, &gameTwo);
        }

        draw_sprite(&(gameCtx.backgroundLeft));
        draw_sprite(&(gameCtx.backgroundRight));
        
        gameCtx.mainTimer.time++;

        #if DEBUG_MODE
            //FntPrint(fnt, "Time: %d\n", get_system_time());

            // Draw and flush the character buffer
            FntFlush(-1);
        #endif

        poll_input(0);
        poll_input(1);

        // Update the display
        display();
    }
    return 0;
}