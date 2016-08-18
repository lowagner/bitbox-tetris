#include "bitbox.h"
#include "common.h"
#include "chiptune.h"
#include "font.h"
#include "palette.h"
#include <stdint.h>
#include <stdlib.h> // abs
#include <string.h> // memset

#define TOP_OFFSET 20
#define BG_COLOR 5
#define BOTTOM_COLOR 132
#define MEMORY(y, x) memory[(y)*2*HOLE_X + (x)]
#define Y_NEXT 3

uint8_t run_paused CCM_MEMORY;

uint32_t top_scores[2] CCM_MEMORY;
uint64_t top_coop_score CCM_MEMORY;
uint64_t top_wide_score CCM_MEMORY;

uint64_t score CCM_MEMORY;
uint32_t scores[2] CCM_MEMORY;

uint8_t instantaneous_lines_cleared CCM_MEMORY;

int8_t game_win_state CCM_MEMORY;

uint8_t field[HOLE_Y][2*HOLE_X] CCM_MEMORY;

struct 
{
    uint8_t tetromino, orientation;
    int8_t y_top, y_bottom; // bounding box
    int8_t x_left, x_right; // for the moving tile
    uint8_t color, dropped;
} 
player[2] CCM_MEMORY;

struct 
{
    uint8_t tetromino, orientation;
    int8_t y_top, y_bottom; // bounding box
    int8_t x_left, x_right; // for the moving tile
    uint8_t color, fill;
} 
next[2] CCM_MEMORY;
    
struct 
{ 
    uint8_t tetromino, orientation;
    int8_t y_top, y_bottom; // bounding box
    int8_t x_left, x_right; // for the moving tile
}
previous[2] CCM_MEMORY;

void run_init()
{
    run_paused = 0; // eventually will probably want this to be 1
}

void load_next_tetromino(int p);

void run_switch()
{
    srand(vga_frame);

    score = 0;
    scores[0] = 0;
    scores[1] = 0;

    chip_play_init(0);
    // also setup play field, up to game_start_height
    memset(field, 0, sizeof(field));
    for (int y=HOLE_Y-game_start_height; y<HOLE_Y; ++y)
    {
        for (int x=0; x<2*HOLE_X; ++x)
            field[y][x] = 1 + rand()%15;
        // pull out one or two random blocks from each side
        field[y][rand()%HOLE_X] = 0;
        field[y][rand()%HOLE_X] = 0;
        field[y][HOLE_X+rand()%HOLE_X] = 0;
        field[y][HOLE_X+rand()%HOLE_X] = 0;
    }
    game_win_state = 0;
    load_next_tetromino(0); // loads cruft into player 0, sets up a good next
    load_next_tetromino(0); // loads actual tetromino into player 0, sets up a next
    load_next_tetromino(1);
    load_next_tetromino(1);
}


void _draw_tetromino
(
    uint8_t tetromino, uint8_t orientation,
    int8_t y_top, int8_t y_bottom, 
    int8_t x_left, int8_t x_right, 
    uint8_t p_color, uint8_t line, 
    int offset, uint32_t *too_far
)
{
    uint32_t *dst = (uint32_t *)draw_buffer + x_left*SQUARE/2 - 1 + offset;
    too_far += offset;
    uint32_t color = palette[p_color]; color |= color<<16;
    switch (tetromino)
    {
        case O4:
            for (int i=0; i<SQUARE/2; ++i)
                *++dst = color;
            if (dst >= too_far)
                dst -= (1+game_wide)*HOLE_X*SQUARE/2;
            for (int i=0; i<SQUARE/2; ++i)
                *++dst = color;
        break;
        case I4:
        if (y_top == y_bottom) // horizontal
        {
            for (int i=0; i<SQUARE/2; ++i)
                *++dst = color;
            if (dst >= too_far)
                dst -= (1+game_wide)*HOLE_X*SQUARE/2;
            for (int i=0; i<SQUARE/2; ++i)
                *++dst = color;
            if (dst >= too_far)
                dst -= (1+game_wide)*HOLE_X*SQUARE/2;
            for (int i=0; i<SQUARE/2; ++i)
                *++dst = color;
            if (dst >= too_far)
                dst -= (1+game_wide)*HOLE_X*SQUARE/2;
            for (int i=0; i<SQUARE/2; ++i)
                *++dst = color;
        }
        else // vertical orientation
        {
            for (int i=0; i<SQUARE/2; ++i)
                *++dst = color;
        }
        break;
        case T4:
        switch (orientation)
        {
            case 0: // nub down
            switch (line)
            {
                case 0:
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                    if (dst >= too_far)
                        dst -= (1+game_wide)*HOLE_X*SQUARE/2;
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                    if (dst >= too_far)
                        dst -= (1+game_wide)*HOLE_X*SQUARE/2;
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                break;
                case 1:
                    dst += SQUARE/2;
                    if (dst >= too_far)
                        dst -= (1+game_wide)*HOLE_X*SQUARE/2;
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                break;
            }
            break;
            case 1: // nub right
            switch (line)
            {
                case 0:
                case 2:
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                break;
                case 1:
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                    if (dst >= too_far)
                        dst -= (1+game_wide)*HOLE_X*SQUARE/2;
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                break;
            }
            break;
            case 2: // nub up
            switch (line)
            {
                case 1:
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                    if (dst >= too_far)
                        dst -= (1+game_wide)*HOLE_X*SQUARE/2;
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                    if (dst >= too_far)
                        dst -= (1+game_wide)*HOLE_X*SQUARE/2;
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                break;
                case 0:
                    dst += SQUARE/2;
                    if (dst >= too_far)
                        dst -= (1+game_wide)*HOLE_X*SQUARE/2;
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                break;
            }
            break;
            case 3: // nub left
            switch (line)
            {
                case 0:
                case 2:
                    dst += SQUARE/2;
                    if (dst >= too_far)
                        dst -= (1+game_wide)*HOLE_X*SQUARE/2;
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                break;
                case 1:
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                    if (dst >= too_far)
                        dst -= (1+game_wide)*HOLE_X*SQUARE/2;
                    for (int i=0; i<SQUARE/2; ++i)
                        *++dst = color;
                break;
            }
            break;
        }
        break;
    }
}

static inline void draw_tetromino(uint8_t p, uint8_t line, int offset)
{
    _draw_tetromino
    (
        player[p].tetromino, player[p].orientation,
        player[p].y_top, player[p].y_bottom, 
        player[p].x_left, player[p].x_right,
        player[p].color, line, offset,
        (uint32_t *)(draw_buffer) + (1+(p|game_wide))*HOLE_X*SQUARE/2 - 1
    );
}

void run_line()
{
    if (vga_line < TOP_OFFSET)
    {
        if (vga_line/2 == 0 || vga_line/2 == (TOP_OFFSET-2)/2)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        else if (vga_line < TOP_OFFSET-10) {}
        else if (run_paused)
        {
            if (vga_frame/32 % 2)
                font_render_line_doubled((const uint8_t *)"paused", 16, vga_line-(TOP_OFFSET-10), 65535, BG_COLOR*257);
        }
        else
            font_render_line_doubled(game_message, 16, vga_line-(TOP_OFFSET-10), 65535, BG_COLOR*257);

        return;
    }
    else if (vga_line >= TOP_OFFSET + HOLE_Y*SQUARE)
    {
        if (vga_line/2 == (TOP_OFFSET + HOLE_Y*SQUARE)/2)
            memset(draw_buffer, BOTTOM_COLOR, 2*SCREEN_W);
        return;
    }

    const int y = (vga_line - TOP_OFFSET)/SQUARE;
    if (game_players == 1)
    {
        int offset;
        if (game_wide)
        {
            offset = 40;
            uint32_t *dst = (uint32_t *)draw_buffer + offset-1;
            for (int x=0; x<HOLE_X; ++x)
            {
                uint8_t block = field[y][x];
                uint32_t color = palette[block&15]; color |= color<<16;
                //block >>= 4;
                for (int j=0; j<SQUARE/2; ++j)
                    *++dst = color;
            }
            for (int x=HOLE_X; x<2*HOLE_X; ++x)
            {
                uint8_t block = field[y][x];
                uint32_t color = palette[block&15]; color |= color<<16;
                //block >>= 4;
                for (int j=0; j<SQUARE/2; ++j)
                    *++dst = color;
            }
        }
        else
        {
            offset = 36;
            uint32_t *dst = (uint32_t *)draw_buffer + offset-1;
            for (int x=0; x<HOLE_X; ++x)
            {
                uint8_t block = field[y][x];
                uint32_t color = palette[block&15]; color |= color<<16;
                //block >>= 4;
                for (int j=0; j<SQUARE/2; ++j)
                    *++dst = color;
            }
        }
        if (y >= player[0].y_top && y <= player[0].y_bottom)
            draw_tetromino(0, y-player[0].y_top, offset);

        offset -= 5*SQUARE/2;
        if (y >= Y_NEXT && y < Y_NEXT+4)
        {
            memset((uint32_t *)draw_buffer+offset, 0, 4*SQUARE*2);
            uint8_t line = y-Y_NEXT;
            if (line <= (next[0].y_bottom - next[0].y_top))
            _draw_tetromino
            (
                next[0].tetromino, next[0].orientation,
                0, next[0].y_bottom - next[0].y_top,
                0, next[0].x_right - next[0].x_left,
                next[0].color, line, offset, (uint32_t *)draw_buffer+128
            );
        }
        else if ((vga_line-TOP_OFFSET)/2 == (Y_NEXT+4)*SQUARE/2)
        {
            memset((uint32_t *)draw_buffer+offset, BG_COLOR, 4*SQUARE*2);
        }
    }
    else
    {
        int offset1, offset2;
        if (game_wide) // cooperative
        {
            offset1 = 30;
            offset2 = 0;
            uint32_t *dst = (uint32_t *)draw_buffer + offset1-1;
            for (int x=0; x<HOLE_X; ++x)
            {
                uint8_t block = field[y][x];
                uint32_t color = palette[block&15]; color |= color<<16;
                //block >>= 4;
                for (int j=0; j<SQUARE/2; ++j)
                    *++dst = color;
            }
            dst += offset2;
            for (int x=HOLE_X; x<2*HOLE_X; ++x)
            {
                uint8_t block = field[y][x];
                uint32_t color = palette[block&15]; color |= color<<16;
                //block >>= 4;
                for (int j=0; j<SQUARE/2; ++j)
                    *++dst = color;
            }
        }
        else // duel
        {
            offset1 = 28;
            offset2 = 5;
            uint32_t *dst = (uint32_t *)draw_buffer + offset1-1;
            for (int x=0; x<HOLE_X; ++x)
            {
                uint8_t block = field[y][x];
                uint32_t color = palette[block&15]; color |= color<<16;
                //block >>= 4;
                for (int j=0; j<SQUARE/2; ++j)
                    *++dst = color;
            }
            dst += offset2;
            for (int x=HOLE_X; x<2*HOLE_X; ++x)
            {
                uint8_t block = field[y][x];
                uint32_t color = palette[block&15]; color |= color<<16;
                //block >>= 4;
                for (int j=0; j<SQUARE/2; ++j)
                    *++dst = color;
            }
        }
        offset2 += offset1;
        if (y >= player[0].y_top && y <= player[0].y_bottom)
            draw_tetromino(0, y-player[0].y_top, offset1);
        if (y >= player[1].y_top && y <= player[1].y_bottom)
            draw_tetromino(1, y-player[1].y_top, offset2);
        offset1 -= 5*SQUARE/2;
        offset2 += (2*HOLE_X+1)*SQUARE/2;
        if (y >= Y_NEXT && y < Y_NEXT+4)
        {
            uint8_t line = y-Y_NEXT;

            memset((uint32_t *)draw_buffer+offset1, 0, 4*SQUARE*2);
            if (line <= (next[0].y_bottom - next[0].y_top))
            _draw_tetromino
            (
                next[0].tetromino, next[0].orientation,
                0, next[0].y_bottom - next[0].y_top,
                0, next[0].x_right - next[0].x_left,
                next[0].color, line, offset1, (uint32_t *)draw_buffer+128
            );
            
            memset((uint32_t *)draw_buffer+offset2, 0, 4*SQUARE*2);
            if (line <= (next[1].y_bottom - next[1].y_top))
            _draw_tetromino
            (
                next[1].tetromino, next[1].orientation,
                0, next[1].y_bottom - next[1].y_top,
                0, next[1].x_right - next[1].x_left,
                next[1].color, line, offset2, (uint32_t *)draw_buffer+128
            );
        }
        else if ((vga_line-TOP_OFFSET)/2 == (Y_NEXT+4)*SQUARE/2)
        {
            memset((uint32_t *)draw_buffer+offset1, BG_COLOR, 4*SQUARE*2);
            memset((uint32_t *)draw_buffer+offset2, BG_COLOR, 4*SQUARE*2);
        }
    }
}

void end_player(int p)
{
    if (game_win_state == 0)
    {
        // if p=1, then player 0 won, make game_win_state even
        // if p=0, then player 1 won, make game_win_state odd
        game_win_state = 3-p + 100;
        if (game_wide)
            strcpy((char *)game_message, "you lost!");
        else
        {
            strcpy((char *)game_message, "player   lost!");
            game_message[7] = '1'+p;
        }
    }
    else if (game_win_state < 0)
        message("already had a tie loss, why is player %d being ended again?\n", p);
    else if (game_win_state % 2 == p)
    {
        strcpy((char *)game_message, "both players lost!");
        game_win_state = -100;
    }
    else
        message("already lost, why is player %d being ended again?\n", p);
}

void load_next_tetromino(int p)
{
    player[p].tetromino = next[p].tetromino;
    player[p].orientation = next[p].orientation;
    player[p].y_top = next[p].y_top;
    player[p].y_bottom = next[p].y_bottom;
    player[p].x_left = next[p].x_left;
    player[p].x_right = next[p].x_right;
    player[p].color = next[p].color;
    player[p].dropped = 0;
   
    next[p].tetromino = rand()%3;
    next[p].color = 1 + rand()%15;

    const int offset = (game_wide && game_players == 1) ? HOLE_X-1 : p*HOLE_X+HOLE_X/2-1;
    switch (next[p].tetromino)
    {
        case O4:
            next[p].y_top = -1;
            next[p].y_bottom = 0;
            next[p].x_left = offset;
            next[p].x_right = offset+1;
        break;
        case I4:
            next[p].orientation = rand()%4;
            if (next[p].orientation % 2) // vertical
            {
                next[p].y_top = -3;
                next[p].y_bottom = 0;
                next[p].x_right = next[p].x_left = offset+rand()%2;
            }
            else // horizontal
            {
                next[p].y_top = 0;
                next[p].y_bottom = 0;
                next[p].x_left = offset-1;
                next[p].x_right = offset+2;
            }
        break;
        case T4:
            next[p].orientation = rand()%4;
            // 0 has nub below, 1 has nub to right, 2 has nub up, 3 has nub left
            if (next[p].orientation % 2) // vertical
            {
                next[p].y_top = -2;
                next[p].y_bottom = 0;
                next[p].x_left = offset;
                next[p].x_right = offset+1;
            }
            else // horizontal
            {
                next[p].y_top = -1;
                next[p].y_bottom = 0;
                next[p].x_left = offset-1+rand()%2;
                next[p].x_right = next[p].x_left+2;
            }
        break;
    }
}

void check_line_clear(int p)
{
    if (game_wide)
    {
        for (int y=player[p].y_top; y<=player[p].y_bottom; ++y)
        {
            int full_line = 1;
            for (int x=0; x<2*HOLE_X; ++x)
            {
                if ((field[y][x]&15) == 0) 
                {
                    full_line = 0;
                    break;
                }
            }
            if (full_line)
            {
                ++instantaneous_lines_cleared;
                for (int z=y; z>0; --z)
                    memcpy(field[z], field[z-1], 2*HOLE_X);
                memset(field[0], 0, 2*HOLE_X);
            }
        }
    }
    else if (p) // duel, player 1 going
    {
        instantaneous_lines_cleared = 0;
        for (int y=player[p].y_top; y<=player[p].y_bottom; ++y)
        {
            int full_line = 1;
            for (int x=HOLE_X; x<2*HOLE_X; ++x)
            {
                if ((field[y][x]&15) == 0) 
                {
                    full_line = 0;
                    break;
                }
            }
            if (full_line)
            {
                ++instantaneous_lines_cleared;
                for (int z=y; z>0; --z)
                    memcpy(&field[z][HOLE_X], &field[z-1][HOLE_X], HOLE_X);
                memset(&field[0][HOLE_X], 0, HOLE_X);
            }
        }
        if (instantaneous_lines_cleared)
        {
            scores[1] += 1 << (instantaneous_lines_cleared*instantaneous_lines_cleared/2);
            if (instantaneous_lines_cleared == 4)
            {
                strcpy((char *)game_message, "tetris!");
                message("player 1 got tetris!\n");
            }
            instantaneous_lines_cleared = 0;
        }
    }
    else // single player, normal level
    {
        instantaneous_lines_cleared = 0;
        for (int y=player[p].y_top; y<=player[p].y_bottom; ++y)
        {
            int full_line = 1;
            for (int x=0; x<HOLE_X; ++x)
            {
                if ((field[y][x]&15) == 0) 
                {
                    full_line = 0;
                    break;
                }
            }
            if (full_line)
            {
                ++instantaneous_lines_cleared;
                for (int z=y; z>0; --z)
                    memcpy(field[z], field[z-1], HOLE_X);
                memset(field[0], 0, HOLE_X);
            }
        }
        if (instantaneous_lines_cleared)
        {
            scores[0] += 1 << (instantaneous_lines_cleared*instantaneous_lines_cleared/2);
            if (instantaneous_lines_cleared == 4)
            {
                strcpy((char *)game_message, "tetris!");
                message("player 0 got tetris!\n");
            }
            instantaneous_lines_cleared = 0;
        }
    }
}

void _set_tetromino(int p, uint8_t *memory)
{
    // assumes that writing to the memory will stay in bounds...
    const int xmax = (1 + (p|game_wide))*HOLE_X - 1;
    const int x0 = (1-game_wide)*p*HOLE_X;
    int x = player[p].x_left;
    int y = player[p].y_top;

    switch (player[p].tetromino)
    {
        case O4:
        {
            // no orientation necessary
            if (x < xmax)
            {
                MEMORY(y, x) = player[p].color;
                MEMORY(y, x+1) = player[p].color;
                MEMORY(y+1, x) = player[p].color;
                MEMORY(y+1, x+1) = player[p].color;
            }
            else
            { 
                MEMORY(y, x0) = player[p].color;
                MEMORY(y, x) = player[p].color;
                MEMORY(y+1, x0) = player[p].color;
                MEMORY(y+1, x) = player[p].color;
            }
        }
        break;
        case I4:
        // could look at orientation, but let's be lazy:
        if (player[p].y_top == player[p].y_bottom) // horizontal
        {
            MEMORY(y, x) = player[p].color;
            if (++x > xmax)
                x = x0;
            MEMORY(y, x) = player[p].color;
            if (++x > xmax)
                x = x0;
            MEMORY(y, x) = player[p].color;
            if (++x > xmax)
                x = x0;
            MEMORY(y, x) = player[p].color;
        }
        else // vertical orientation
        {
            MEMORY(y, x) = player[p].color;
            MEMORY(y+1, x) = player[p].color;
            MEMORY(y+2, x) = player[p].color;
            MEMORY(y+3, x) = player[p].color;
        }
        break;
        case T4:
        switch (player[p].orientation)
        {
            case 0: // nub down
                MEMORY(y, x) = player[p].color;
                if (++x > xmax)
                    x = x0;
                MEMORY(y, x) = player[p].color;
                MEMORY(y+1, x) = player[p].color;
                if (++x > xmax)
                    x = x0;
                MEMORY(y, x) = player[p].color;
            break;
            case 1: // nub right
                MEMORY(y, x) = player[p].color;
                MEMORY(y+1, x) = player[p].color;
                MEMORY(y+2, x) = player[p].color;
                if (++x > xmax)
                    x = x0;
                MEMORY(y+1, x) = player[p].color;
            break;
            case 2: // nub up
                MEMORY(y+1, x) = player[p].color;
                if (++x > xmax)
                    x = x0;
                MEMORY(y+1, x) = player[p].color;
                MEMORY(y, x) = player[p].color;
                if (++x > xmax)
                    x = x0;
                MEMORY(y+1, x) = player[p].color;
            break;
            case 3: // nub left
                MEMORY(y+1, x) = player[p].color;
                if (++x > xmax)
                    x = x0;
                MEMORY(y, x) = player[p].color;
                MEMORY(y+1, x) = player[p].color;
                MEMORY(y+2, x) = player[p].color;
            break;
        }
        break;
        case L4:

        break;
        case J4:

        break;
        case S4:

        break;
        case Z4:

        break;
        default:
            message("unknown tetromino for player 0: %d\n", player[0].tetromino);
    }
}

int _check_all(int p, uint8_t *memory)
{
    const int xmax = (1 + (p|game_wide))*HOLE_X - 1;
    const int x0 = (1-game_wide)*p*HOLE_X;
    int x = player[p].x_left;
    int y = player[p].y_top;
    switch (player[p].tetromino)
    {
        case O4:
        {
            // no orientation necessary
            if (x < xmax)
            {
                if (MEMORY(y, x) & 15)
                    return 1;
                if (MEMORY(y, x+1) & 15)
                    return 1;
                if (MEMORY(y+1, x) & 15)
                    return 1;
                if (MEMORY(y+1, x+1) & 15)
                    return 1;
            }
            else
            {
                if (MEMORY(y, x0) & 15)
                    return 1;
                if (MEMORY(y, x) & 15)
                    return 1;
                if (MEMORY(y+1, x0) & 15)
                    return 1;
                if (MEMORY(y+1, x) & 15)
                    return 1;
            }
        }
        break;
        case I4:
        // could look at orientation, but let's be lazy:
        if (player[p].y_top == player[p].y_bottom) // horizontal
        {
            if (MEMORY(y, x) & 15)
                return 1;
            if (++x > xmax)
                x = x0;
            if (MEMORY(y, x) & 15)
                return 1;
            if (++x > xmax)
                x = x0;
            if (MEMORY(y, x) & 15)
                return 1;
            if (++x > xmax)
                x = x0;
            if (MEMORY(y, x) & 15)
                return 1;
        }
        else // vertical orientation
        {
            if (MEMORY(y, x) & 15)
                return 1;
            if (MEMORY(y+1, x) & 15)
                return 1;
            if (MEMORY(y+2, x) & 15)
                return 1;
            if (MEMORY(y+3, x) & 15)
                return 1;
        }
        break;
        case T4:
        switch (player[p].orientation)
        {
            case 0: // nub down
                if (MEMORY(y, x) & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (MEMORY(y, x) & 15)
                    return 1;
                if (MEMORY(y+1, x) & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (MEMORY(y, x) & 15)
                    return 1;
            break;
            case 1: // nub right
                if (MEMORY(y, x) & 15)
                    return 1;
                if (MEMORY(y+1, x) & 15)
                    return 1;
                if (MEMORY(y+2, x) & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (MEMORY(y+1, x) & 15)
                    return 1;
            break;
            case 2: // nub up
                if (MEMORY(y+1, x) & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (MEMORY(y+1, x) & 15)
                    return 1;
                if (MEMORY(y, x) & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (MEMORY(y+1, x) & 15)
                    return 1;
            break;
            case 3: // nub left
                if (MEMORY(y+1, x) & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (MEMORY(y, x) & 15)
                    return 1;
                if (MEMORY(y+1, x) & 15)
                    return 1;
                if (MEMORY(y+2, x) & 15)
                    return 1;
            break;
        }
        break;
        case L4:

        break;
        case J4:

        break;
        case S4:

        break;
        case Z4:

        break;
        default:
            message("unknown tetromino for player 0: %d\n", player[0].tetromino);
    }
    return 0;
}

int check_left(int p)
{
    const int xmax = (1 + (p|game_wide))*HOLE_X - 1;
    const int x0 = (1-game_wide)*p*HOLE_X;
    int x = player[p].x_left;
    int y = player[p].y_top;
    switch (player[p].tetromino)
    {
        case O4:
        {
            // no orientation necessary
            if (field[y][x] & 15)
                return 1;
            if (field[y+1][x] & 15)
                return 1;
        }
        break;
        case I4:
        // could look at orientation, but let's be lazy:
        if (player[p].y_top == player[p].y_bottom) // horizontal
        {
            if (field[y][x] & 15)
                return 1;
        }
        else // vertical orientation
        {
            if (field[y][x] & 15)
                return 1;
            if (field[y+1][x] & 15)
                return 1;
            if (field[y+2][x] & 15)
                return 1;
            if (field[y+3][x] & 15)
                return 1;
        }
        break;
        case T4:
        switch (player[p].orientation)
        {
            case 0: // nub down
                if (field[y][x] & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (field[y+1][x] & 15)
                    return 1;
            break;
            case 1: // nub right
                if (field[y][x] & 15)
                    return 1;
                if (field[y+1][x] & 15)
                    return 1;
                if (field[y+2][x] & 15)
                    return 1;
            break;
            case 2: // nub up
                if (field[y+1][x] & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (field[y][x] & 15)
                    return 1;
            break;
            case 3: // nub left
                if (field[y+1][x] & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (field[y][x] & 15)
                    return 1;
                if (field[y+2][x] & 15)
                    return 1;
            break;
        }
        break;
        case L4:

        break;
        case J4:

        break;
        case S4:

        break;
        case Z4:

        break;
        default:
            message("unknown tetromino for player 0: %d\n", player[0].tetromino);
    }
    return 0;
}

int check_right(int p)
{
    const int xmax = (1 + (p|game_wide))*HOLE_X - 1;
    const int x0 = (1-game_wide)*p*HOLE_X;
    int x = player[p].x_right;
    int y = player[p].y_top;
    switch (player[p].tetromino)
    {
        case O4:
        {
            // no orientation necessary
            if (field[y][x] & 15)
                return 1;
            if (field[y+1][x] & 15)
                return 1;
        }
        break;
        case I4:
        // could look at orientation, but let's be lazy:
        if (player[p].y_top == player[p].y_bottom) // horizontal
        {
            if (field[y][x] & 15)
                return 1;
        }
        else // vertical orientation
        {
            if (field[y][x] & 15)
                return 1;
            if (field[y+1][x] & 15)
                return 1;
            if (field[y+2][x] & 15)
                return 1;
            if (field[y+3][x] & 15)
                return 1;
        }
        break;
        case T4:
        switch (player[p].orientation)
        {
            case 0: // nub down
                if (field[y][x] & 15)
                    return 1;
                if (--x < x0)
                    x = xmax;
                if (field[y+1][x] & 15)
                    return 1;
            break;
            case 1: // nub right
                if (field[y+1][x] & 15)
                    return 1;
                if (--x < x0)
                    x = xmax;
                if (field[y][x] & 15)
                    return 1;
                if (field[y+2][x] & 15)
                    return 1;
            break;
            case 2: // nub up
                if (field[y+1][x] & 15)
                    return 1;
                if (--x < x0)
                    x = xmax;
                if (field[y][x] & 15)
                    return 1;
            break;
            case 3: // nub left
                if (field[y][x] & 15)
                    return 1;
                if (field[y+1][x] & 15)
                    return 1;
                if (field[y+2][x] & 15)
                    return 1;
            break;
        }
        break;
        case L4:

        break;
        case J4:

        break;
        case S4:

        break;
        case Z4:

        break;
        default:
            message("unknown tetromino for player 0: %d\n", player[0].tetromino);
    }
    return 0;
}

int check_bottom(int p)
{
    // check collision, and if collided, move the tetromino up and set it
    const int xmax = (1 + (p|game_wide))*HOLE_X - 1;
    const int x0 = (1-game_wide)*p*HOLE_X;
    int x = player[p].x_left;
    int y = player[p].y_bottom;
    switch (player[p].tetromino)
    {
        case O4:
        {
            // no orientation necessary
            if (field[y][x] & 15)
                return 1;
            if (++x > xmax)
                x = x0;
            if (field[y][x] & 15)
                return 1;
        }
        break;
        case I4:
        // could look at orientation, but let's be lazy:
        if (player[p].y_top == player[p].y_bottom) // horizontal
        {
            if (field[y][x] & 15)
                return 1;
            if (++x > xmax)
                x = x0;
            if (field[y][x] & 15)
                return 1;
            if (++x > xmax)
                x = x0;
            if (field[y][x] & 15)
                return 1;
            if (++x > xmax)
                x = x0;
            if (field[y][x] & 15)
                return 1;
        }
        else // vertical orientation
        {
            if (field[y][x] & 15)
                return 1;
        }
        break;
        case T4:
        switch (player[p].orientation)
        {
            case 0: // nub down
                if (field[y-1][x] & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (field[y][x] & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (field[y-1][x] & 15)
                    return 1;
            break;
            case 1: // nub right
                if (field[y][x] & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (field[y-1][x] & 15)
                    return 1;
            break;
            case 2: // nub up
                if (field[y][x] & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (field[y][x] & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (field[y][x] & 15)
                    return 1;
            break;
            case 3: // nub left
                if (field[y-1][x] & 15)
                    return 1;
                if (++x > xmax)
                    x = x0;
                if (field[y][x] & 15)
                    return 1;
            break;
        }
        break;
        case L4:

        break;
        case J4:

        break;
        case S4:

        break;
        case Z4:

        break;
        default:
            message("unknown tetromino for player 0: %d\n", player[0].tetromino);
    }
    return 0;
}

static inline void set_tetromino(int p)
{
    // set player p's tetromino in stone, wherever it is
    if (player[p].y_top < 0)
        return end_player(p);

    _set_tetromino(p, field[0]);

    check_line_clear(p);
    load_next_tetromino(p);
}

int check_boundaries(int p)
{
    // check new orientation, return 1 if out of bounds
    if (player[p].y_bottom >= HOLE_Y)
        return 1;
    else if (game_wide)
    {
        if (game_torus)
        {
            if (player[p].x_left < 0)
                player[p].x_left += 2*HOLE_X;
            else if (player[p].x_left >= 2*HOLE_X)
                player[p].x_left -= 2*HOLE_X;
            if (player[p].x_right < 0)
                player[p].x_right += 2*HOLE_X;
            else if (player[p].x_right >= 2*HOLE_X)
                player[p].x_right -= 2*HOLE_X;
        }
        else
        {
            if (player[p].x_left < 0)
                return 1;
            else if (player[p].x_left >= 2*HOLE_X)
                return 1;
            if (player[p].x_right < 0)
                return 1;
            else if (player[p].x_right >= 2*HOLE_X)
                return 1;
        }
    }
    else if (p) // player 1 by him/herself
    {
        if (game_torus)
        {
            if (player[1].x_left < 0)
                player[1].x_left += HOLE_X;
            else if (player[1].x_left >= HOLE_X)
                player[1].x_left -= HOLE_X;
            if (player[1].x_right < 0)
                player[1].x_right += HOLE_X;
            else if (player[1].x_right >= HOLE_X)
                player[1].x_right -= HOLE_X;
        }
        else
        {
            if (player[1].x_left < 0)
                return 1;
            else if (player[1].x_left >= HOLE_X)
                return 1;
            if (player[1].x_right < 0)
                return 1;
            else if (player[1].x_right >= HOLE_X)
                return 1;
        }
    }
    else // player 0 by him/herself
    {
        if (game_torus)
        {
            if (player[0].x_left < 0)
                player[0].x_left += HOLE_X;
            else if (player[0].x_left >= HOLE_X)
                player[0].x_left -= HOLE_X;
            if (player[0].x_right < 0)
                player[0].x_right += HOLE_X;
            else if (player[0].x_right >= HOLE_X)
                player[0].x_right -= HOLE_X;
        }
        else
        {
            if (player[0].x_left < 0)
                return 1;
            else if (player[0].x_left >= HOLE_X)
                return 1;
            if (player[0].x_right < 0)
                return 1;
            else if (player[0].x_right >= HOLE_X)
                return 1;
        }
    }

    return 0;
}

static inline int check_all(int p)
{
    return _check_all(p, field[0]);
}

int check_players()
{
    // assume a two player game
    // return 0 if players are not overlapping
    // return 1 if players are overlapping

    if (!game_wide) // no chance for overlap
        return 0;

    if (player[0].y_top > player[1].y_bottom ||
        player[0].y_bottom < player[1].y_top)
        return 0;

    if (game_torus)
    {
        // for a torus, don't be fancy, just check it all...
        // TODO: something fancy 
    }
    else
    if ((player[0].x_left > player[1].x_right) ||
        (player[0].x_right < player[1].x_left))
        return 0;

    uint8_t work[7][2*HOLE_X];
    memset(work, 0, sizeof(work));
    int y0 = player[0].y_top < player[1].y_top ? player[0].y_top : player[1].y_top;

    player[0].y_top -= y0;
    player[0].y_bottom -= y0;
    _set_tetromino(0, work[0]);
    player[0].y_top += y0;
    player[0].y_bottom += y0;

    player[1].y_top -= y0;
    player[1].y_bottom -= y0;
    int collide = _check_all(1, work[0]);
    player[1].y_top += y0;
    player[1].y_bottom += y0;
     
    return collide;
}

int collide_bottom()
{
    instantaneous_lines_cleared = 0;

    int collided[2] = { 0, 0 };
    collided[0] = (player[0].y_bottom == HOLE_Y) || check_bottom(0);
    
    if (collided[0])
    {
        --player[0].y_top;
        --player[0].y_bottom;
        set_tetromino(0);
    }
    
    if (game_players > 1)
    {
        collided[1] = (player[1].y_bottom == HOLE_Y) || check_bottom(1);

        if (collided[1])
        {
            --player[1].y_top;
            --player[1].y_bottom;
            set_tetromino(1);
            
            if (game_wide && collided[0] == 0)
            {
                // double check if player 0 hit on top of player 1
                if (check_bottom(0))
                {
                    --player[0].y_top;
                    --player[0].y_bottom;
                    set_tetromino(0);
                }
            }
        }
    }

    if (instantaneous_lines_cleared)
    {
        score += (17-8*game_players) << (instantaneous_lines_cleared*instantaneous_lines_cleared/2);
        if (instantaneous_lines_cleared == 4)
            strcpy((char *)game_message, "tetris!");
        else if (instantaneous_lines_cleared == 8)
            strcpy((char *)game_message, "octris!!");
        else if (instantaneous_lines_cleared > 4)
            strcpy((char *)game_message, "awesome!");
        else
            game_message[0] = 0;
        message("got lines %d - points %d, got score %lu\n", instantaneous_lines_cleared, ((17-8*game_players)<<(instantaneous_lines_cleared*instantaneous_lines_cleared/2)), score);
    }
    
    return collided[0] | (collided[1]<<1);
}

void raise_up()
{
    int x=0;
    // check top row for any blocks
    for (; x<HOLE_X; ++x)
    {
        if (field[0][x]&15)
            end_player(0);
    }
    if (game_players == 2)
    for (; x<2*HOLE_X; ++x)
    {
        if (field[0][x]&15)
            end_player(1);
    }
    // now move all rows up
    int y; // destination row, y+1 is the source row
    for (y=0; y<HOLE_Y-1; ++y)
    {
        for (x=0; x<2*HOLE_X; ++x)
            field[y][x] = field[y+1][x];
    }
    for (x=0; x<2*HOLE_X; ++x)
        field[y][x] = 1 + rand()%15;
    // pull out one or two random blocks from each side
    field[y][rand()%HOLE_X] = 0;
    field[y][rand()%HOLE_X] = 0;
    field[y][HOLE_X+rand()%HOLE_X] = 0;
    field[y][HOLE_X+rand()%HOLE_X] = 0;
    // check collisions:  if any player blocks are on top of field blocks,
    // push the player block up in y, set it, and put in a new one
    collide_bottom();
}

void reset_position(int p)
{
    player[p].orientation = previous[p].orientation;
    player[p].y_top = previous[p].y_top;
    player[p].y_bottom = previous[p].y_bottom;
    player[p].x_left = previous[p].x_left;
    player[p].x_right = previous[p].x_right;
}

void save_position(int p)
{
    previous[p].tetromino = player[p].tetromino;
    previous[p].orientation = player[p].orientation;
    previous[p].y_top = player[p].y_top;
    previous[p].y_bottom = player[p].y_bottom;
    previous[p].x_left = player[p].x_left;
    previous[p].x_right = player[p].x_right;
}

int rotate_clockwise(int p)
{
    // return zero if not rotated.
    switch (player[p].tetromino)
    {
        case O4:
            return 0;
        break;
        case I4:
            switch (player[p].orientation)
            {
                case 0: // horizontal, pivot left of center
                    player[p].x_right = ++player[p].x_left;
                    player[p].y_top -= 1;
                    player[p].y_bottom += 2;

                    player[p].orientation = 3;
                break;
                case 1: // vertical, pivot below center
                    player[p].y_top = --player[p].y_bottom;
                    player[p].x_left -= 1;
                    player[p].x_right += 2;

                    player[p].orientation = 0;
                break;
                case 2: // horizontal, pivot right of center
                    player[p].x_left = --player[p].x_right;
                    player[p].y_top -= 2;
                    player[p].y_bottom += 1;

                    player[p].orientation = 1;
                break;
                case 3: // vertical, pivot above center
                    player[p].y_bottom = ++player[p].y_top;
                    player[p].x_left -= 2;
                    player[p].x_right += 1;

                    player[p].orientation = 2;
                break;
            }
        break;
        case T4:
            switch (player[p].orientation)
            {
                case 0: // nub below
                    --player[p].y_top;
                    --player[p].x_right;

                    player[p].orientation = 3;
                break;
                case 1: // nub right
                    ++player[p].y_top;
                    --player[p].x_left;

                    player[p].orientation = 0;
                break;
                case 2: // nub up
                    ++player[p].y_bottom;
                    ++player[p].x_left;

                    player[p].orientation = 1;
                break;
                case 3: // nub left
                    --player[p].y_bottom;
                    ++player[p].x_right;

                    player[p].orientation = 2;
                break;
            }
        break;
        case L4:

        break;
        case J4:

        break;
        case S4:

        break;
        case Z4:

        break;
        default:
            message("unknown tetromino for player 0: %d\n", player[0].tetromino);
    }
    if (check_boundaries(p))
    {
        reset_position(p);
        return 0;
    }
    return 0;
}

int rotate_counterclockwise(int p)
{
    // perform a check on HOLE sides and bottom; do not rotate if not allowed.
    // DO NOT CHECK for blocks, those will be checked later.
    // return 1 if rotated, 0 if not.
    switch (player[p].tetromino)
    {
        case O4:
            return 0;
        break;
        case I4:
            switch (player[p].orientation)
            {
                case 0: // horizontal, pivot left of center
                    player[p].x_right = ++player[p].x_left;
                    player[p].y_top -= 2;
                    player[p].y_bottom += 1;

                    player[p].orientation = 1;
                break;
                case 1: // vertical, pivot below center
                    player[p].y_top = --player[p].y_bottom;
                    player[p].x_left -= 2;
                    player[p].x_right += 1;

                    player[p].orientation = 2;
                break;
                case 2: // horizontal, pivot right of center
                    player[p].x_left = --player[p].x_right;
                    player[p].y_top -= 1;
                    player[p].y_bottom += 2;

                    player[p].orientation = 3;
                break;
                case 3: // vertical, pivot above center
                    player[p].y_bottom = ++player[p].y_top;
                    player[p].x_left -= 1;
                    player[p].x_right += 2;

                    player[p].orientation = 0;
                break;
            }
        break;
        case T4:
            switch (player[p].orientation)
            {
                case 0: // nub below
                    --player[p].y_top;
                    ++player[p].x_left;

                    player[p].orientation = 1;
                break;
                case 1: // nub right
                    --player[p].y_bottom;
                    --player[p].x_left;

                    player[p].orientation = 2;
                break;
                case 2: // nub up
                    ++player[p].y_bottom;
                    --player[p].x_right;

                    player[p].orientation = 3;
                break;
                case 3: // nub left
                    ++player[p].y_top;
                    ++player[p].x_right;

                    player[p].orientation = 0;
                break;
            }
        break;
        case L4:

        break;
        case J4:

        break;
        case S4:

        break;
        case Z4:

        break;
        default:
            message("unknown tetromino for player 0: %d\n", player[0].tetromino);
    }
    if (check_boundaries(p))
    {
        reset_position(p);
        return 0;
    }
    return 1;
}

void handle_movement()
{
    // handle lateral movement

    int collided[2] = {0, 0};
    
    save_position(0);

    if (GAMEPAD_PRESSED(0, down))
        player[0].dropped = 1;

    if (GAMEPAD_PRESS(0, R) || GAMEPAD_PRESS(0, A))
    {
        if (rotate_clockwise(0))
            collided[0] = check_all(0);
    }
    else if (GAMEPAD_PRESS(0, L) || GAMEPAD_PRESS(0, B) || GAMEPAD_PRESS(0, up))
    {
        if (rotate_counterclockwise(0))
            collided[0] = check_all(0);
    }
    if (GAMEPAD_PRESS(0, left))
    {
        if (player[0].x_left > 0)
        {
            --player[0].x_left;
            --player[0].x_right;
            if (game_torus && player[0].x_right < 0)
                player[0].x_right = (1+game_wide)*HOLE_X-1;
        }
        else if (game_torus)
            player[0].x_left = (1+game_wide)*HOLE_X-1;
        else
            goto handle_next;

        if (collided[0])
            collided[0] = check_all(0);
        else
            collided[0] = check_left(0);
    }
    else if (GAMEPAD_PRESS(0, right))
    {
        if (player[0].x_right < (1+game_wide)*HOLE_X-1)
        {
            ++player[0].x_left;
            ++player[0].x_right;
            if (game_torus && player[0].x_left >= (1+game_wide)*HOLE_X)
                player[0].x_left = 0;
        }
        else if (game_torus)
            player[0].x_right = 0;
        else
            goto handle_next;
        
        collided[0] = check_right(0);
        if (collided[0])
            collided[0] = check_all(0);
        else
            collided[0] = check_left(0);
    }

    handle_next:
    if (collided[0])
        reset_position(0);

    if (game_players == 1)
        return;

    save_position(1);
    
    if (GAMEPAD_PRESSED(1, down))
        player[1].dropped = 1;
    
    if (GAMEPAD_PRESS(1, R) || GAMEPAD_PRESS(1, A))
    {
        if (rotate_clockwise(1))
            collided[1] = check_all(1);
    }
    else if (GAMEPAD_PRESS(1, L) || GAMEPAD_PRESS(1, B) || GAMEPAD_PRESS(1, up))
    {
        if (rotate_counterclockwise(1))
            collided[1] = check_all(1);
    }
    if (GAMEPAD_PRESS(1, left))
    {
        if (player[1].x_left > (1-game_wide)*HOLE_X)
        {
            --player[1].x_left;
            --player[1].x_right;
            if (game_torus && player[1].x_right < (1-game_wide)*HOLE_X)
                player[1].x_right = 2*HOLE_X-1;
        }
        else if (game_torus)
            player[1].x_left = 2*HOLE_X-1;
        else
            goto handle_both;
        
        if (collided[1])
            collided[1] = check_all(1);
        else
            collided[1] = check_left(1);
    }
    else if (GAMEPAD_PRESS(1, right))
    {
        if (player[1].x_right < 2*HOLE_X-1)
        {
            ++player[1].x_left;
            ++player[1].x_right;
            if (game_torus && player[1].x_left >= 2*HOLE_X)
                player[1].x_left = (1-game_wide)*HOLE_X;
        }
        else if (game_torus)
            player[1].x_right = (1-game_wide)*HOLE_X;
        else
            goto handle_both;

        if (collided[1])
            collided[1] = check_all(1);
        else
            collided[1] = check_right(1);
    }

    handle_both:
    if (collided[1])
        reset_position(1);

    // check collisions between players
    if (check_players())
    {
        reset_position(0);
        reset_position(1);
    }
}

void move_down_maybe()
{
    // bring players down, based on timing and if they're holding down.
    if (game_fall_speed)
    {
        if ((vga_frame%4 == 0) &&
            ((vga_frame/4) % (10-game_fall_speed) == 0))
        {
            ++player[0].y_top;
            ++player[0].y_bottom;
           
            ++player[1].y_top;
            ++player[1].y_bottom;
            
            collide_bottom();
        }
        else if (vga_frame%2 == 0)
        {
            int moved = 0;

            if (GAMEPAD_PRESSED(0, down))
            {
                ++player[0].y_top;
                ++player[0].y_bottom;
                moved = 1;
            }

            if (game_players == 1)
            {
                if (moved)
                    collide_bottom();
                return;
            }

            if (GAMEPAD_PRESSED(1, down))
            {
                ++player[1].y_top;
                ++player[1].y_bottom;
                moved += 2;
            }

            if (moved && collide_bottom() == 0)
            {
                if (check_players())
                switch (moved)
                {
                    case 3:
                        message("unexpected moved=3 in fallspeed>0 mode\n");
                    case 1:
                        --player[0].y_top;
                        --player[0].y_bottom;
                    break;
                    case 2:
                        --player[1].y_top;
                        --player[1].y_bottom;
                    break;
                }
            }
        }
    }
    else // game_fall_speed = 0
    {
        int moved = 0;
        
        if (player[0].dropped || player[0].y_top < 0)
        {
            ++player[0].y_top;
            ++player[0].y_bottom;
            moved = 1;
        }
        if (player[1].dropped || player[1].y_top < 0)
        {
            ++player[1].y_top;
            ++player[1].y_bottom;
            moved += 2;
        }

        if (moved && collide_bottom() == 0)
        {
            // neither collided with something solid, 
            // check if one collided with the other
            if (check_players())
            switch (moved)
            {
                case 3:
                    message("unexpected moved=3 in fallspeed=0 mode\n");
                    // fall through to resolve...?
                case 1:
                    --player[0].y_top;
                    --player[0].y_bottom;
                break;
                case 2:
                    --player[1].y_top;
                    --player[1].y_bottom;
                break;
            }
        }
    }
}

void run_controls()
{
    if (GAMEPAD_PRESS(0, start))
    {
        if (GAMEPAD_PRESSING(0, select) || game_win_state)
        {
            game_message[0] = 0;
            run_paused = 0;
            previous_visual_mode = None;
            game_switch(MainMenu);
            return;
        }
        // pause mode
        chip_play = run_paused;
        run_paused = 1 - run_paused;
        return;
    }

    if (run_paused)
        return;

    if (game_win_state)
    {
        if (game_win_state < 0) // both players lost
        {
            if (game_win_state == -1)
                game_switch(MainMenu); // return to menu
            else if (vga_frame % 8 == 0)
                ++game_win_state;
        }
        else if (game_win_state > 2) // even: player 0 won; odd: player 1 won
        {
            if (vga_frame % 8 == 0)
                game_win_state -= 2;
        }
        else
            game_switch(MainMenu);
        return;
    }

    move_down_maybe();

    handle_movement();

    if ((game_raise_speed) && (vga_frame%512 == 0) && 
        ((vga_frame/512) % (10-game_raise_speed) == 0))
        raise_up();
}
