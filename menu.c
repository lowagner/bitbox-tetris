#include "bitbox.h"
#include "common.h"
#include "chiptune.h"
#include "font.h"
#include "name.h"
#include "io.h"
#include <stdint.h>
#include <stdlib.h> // abs
#include <string.h> // memset

#define BG_COLOR 141
#define SELECT_COLOR RGB(255,150,250)
#define MENU_OPTIONS 8
#define NUMBER_LINES 19

uint8_t menu_index CCM_MEMORY;
uint8_t game_players CCM_MEMORY;
uint8_t game_wide CCM_MEMORY;
uint8_t game_fall_speed CCM_MEMORY;
uint8_t game_raise_speed CCM_MEMORY;
uint8_t game_start_height CCM_MEMORY;
uint8_t game_torus CCM_MEMORY;

void menu_init()
{
    menu_index = 0;
    game_players = 2;
    game_wide = 1;
    game_fall_speed = 5;
    game_raise_speed = 1;
    game_start_height = 1;
    game_torus = 1;
}

void menu_line()
{
    if (vga_line < 16)
    {
        if (vga_line/2 == 0)
            memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    else if (vga_line >= 16 + NUMBER_LINES*10)
        return;

    int line = vga_line-16;
    int internal_line = line%10;
    line /= 10;
    if (internal_line >= 8)
    {
        memset(draw_buffer, BG_COLOR, 2*SCREEN_W);
        return;
    }
    switch (line)
    {
        case 2:
            font_render_line_doubled((const uint8_t *)"TETRIBOX", 34, internal_line, 65535, BG_COLOR*257);
        break;
        case 4:
        {
            uint8_t msg[] = { '0'+game_players, ' ', 'p', 'l', 'a', 'y', 'e', 'r', 0 };
            font_render_line_doubled(msg, 52, internal_line, menu_index == 0 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        break;
        case 5:
        if (game_players == 1)
        {
            if (game_wide)
            font_render_line_doubled((const uint8_t *)"wide", 52, internal_line, menu_index == 1 ? SELECT_COLOR :
                65535, BG_COLOR*257);
            else
            font_render_line_doubled((const uint8_t *)"normal", 52, internal_line, menu_index == 1 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        else
        {
            if (game_wide)
            font_render_line_doubled((const uint8_t *)"coop", 52, internal_line, menu_index == 1 ? SELECT_COLOR :
                65535, BG_COLOR*257);
            else
            font_render_line_doubled((const uint8_t *)"duel", 52, internal_line, menu_index == 1 ? SELECT_COLOR :
                65535, BG_COLOR*257);

        }
        break;
        case 7:
        {
            uint8_t msg[] = { 'f', 'a', 'l', 'l', ' ', 's', 'p', 'e', 'e', 'd', ' ', 
                '0'+game_fall_speed%10, 
            0 };
            font_render_line_doubled(msg, 52, internal_line, menu_index == 2 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        break;
        case 8:
        {
            uint8_t msg[] = { 'r', 'a', 'i', 's', 'e', ' ', 's', 'p', 'e', 'e', 'd', ' ', 
                '0'+game_raise_speed%10, 
            0 };
            font_render_line_doubled(msg, 52, internal_line, menu_index == 3 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        break;
        case 9:
        {
            uint8_t msg[] = { 's', 't', 'a', 'r', 't', ' ', 'h', 'e', 'i', 'g', 'h', 't', ' ', 
                '0'+game_start_height%10, 
            0 };
            font_render_line_doubled(msg, 52, internal_line, menu_index == 4 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        break;
        case 10:
        if (game_torus)
        {
            font_render_line_doubled((const uint8_t *)"torus", 52, internal_line, menu_index == 5 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        else
        {
            font_render_line_doubled((const uint8_t *)"walls", 52, internal_line, menu_index == 5 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        break;
        case 12:
        if (available_count)
        {
            uint8_t msg[24] = { 'm', 'u', 's', 'i', 'c', ' ' };
            strcpy((char *)msg+6, available_filenames[available_index]);
            font_render_line_doubled(msg, 52, internal_line, menu_index == 6 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        break;
        case 13:
        if (available_count)
        {
            uint8_t msg[] = { 'v', 'o', 'l', 'u', 'm', 'e', ' ', 
                hex[chip_volume/16], hex[chip_volume % 16], 
            0 };
            font_render_line_doubled(msg, 52, internal_line, menu_index == 7 ? SELECT_COLOR :
                65535, BG_COLOR*257);
        }
        break;
        case 16:
            font_render_line_doubled((const uint8_t *)"dpad:change options", 44, internal_line, 65535, BG_COLOR*257);
        break;
        case 17:
            font_render_line_doubled((const uint8_t *)"select:music test", 44, internal_line, 65535, BG_COLOR*257);
        break;
        case 18:
            font_render_line_doubled((const uint8_t *)"start:play", 44, internal_line, 65535, BG_COLOR*257);
        break;
    }
}

void load_song(int init_also)
{
    chip_kill();
    if (strcmp(base_filename, available_filenames[available_index]))
    {
        strcpy(base_filename, available_filenames[available_index]);
        io_load_instrument(16);
        io_load_verse(16);
        io_load_anthem();
    }
    if (init_also)
        chip_play_init(0);
}

void menu_controls()
{
    if (GAMEPAD_PRESS(0, down))
    {
        ++menu_index;
        if (available_count)
        {
            if (menu_index >= MENU_OPTIONS)
                menu_index = 0;
        }
        else
        {
            if (menu_index >= MENU_OPTIONS-2)
                menu_index = 0;
        }
    }
    if (GAMEPAD_PRESS(0, up))
    {
        if (menu_index)
            --menu_index;
        else if (available_count)
            menu_index = MENU_OPTIONS-1;
        else
            menu_index = MENU_OPTIONS-3;
    }
    int modified = 0;
    if (GAMEPAD_PRESS(0, right))
        ++modified;
    if (GAMEPAD_PRESS(0, left))
        --modified;
    if (modified)
    {
        switch (menu_index)
        {
        case 0: // player
            game_players = 3 - game_players;
        break;
        case 1:
            game_wide = 1 - game_wide;
        break;
        case 2:
            game_fall_speed += modified;
            if (game_fall_speed > 128)
                game_fall_speed = 9;
            else if (game_fall_speed > 9)
                game_fall_speed = 0;
        break;
        case 3:
            game_raise_speed += modified;
            if (game_raise_speed > 128)
                game_raise_speed = 9;
            else if (game_raise_speed > 9)
                game_raise_speed = 0;
        break;
        case 4:
            game_start_height += modified;
            if (game_start_height > 128)
                game_start_height = 9;
            else if (game_start_height > 9)
                game_start_height = 0;
        break;
        case 5:
            game_torus = 1 - game_torus;
        break;
        case 6: // switch song
            if (available_count == 0)
                break;
            chip_kill();
            if (modified > 0)
            {
                if (++available_index >= available_count)
                    available_index = 0;
            }
            else if (available_index)
                --available_index;
            else
                available_index = available_count-1;
        break;
        case 7: // switch volume
            if (modified > 0)
            {
                if (chip_volume < 240)
                    chip_volume += 16;
                else if (chip_volume < 255)
                    ++chip_volume;
            }
            else if (chip_volume >= 16)
                chip_volume -= 16;
            else if (chip_volume)
                --chip_volume;
        break;
        }
    }
    if (GAMEPAD_PRESS(0, start))
    {
        // start game
        load_song(0); // load song but don't play
        previous_visual_mode = MainMenu;
        if (GAMEPAD_PRESSING(0, L))
            game_switch(EditAnthem);
        else
            game_switch(GameOn);
        return;
    }
    if (GAMEPAD_PRESS(0, select))
    {
        load_song(1); // load and play
        return;
    }
}
