#include "bitbox.h"
#include "common.h"
#include "chiptune.h"
#include "instrument.h"
#include "verse.h"
#include "anthem.h"
#include "name.h"
#include "io.h"
#include "font.h"
#include "palette.h"
#include "menu.h"
#include "run.h"

#include "string.h" // memcpy

VisualMode visual_mode CCM_MEMORY; 
VisualMode previous_visual_mode CCM_MEMORY;
VisualMode old_visual_mode CCM_MEMORY;
uint8_t game_message[32] CCM_MEMORY;
uint16_t old_gamepad[2] CCM_MEMORY;
uint8_t gamepad_press_wait;

#define BSOD 140

void game_init()
{ 
    game_message[0] = 0;
    menu_init();
    font_init();
    anthem_init();
    verse_init();
    chip_init();
    instrument_init();
    palette_init();

    // now load everything else
    if (io_get_recent_filename())
    {
        message("resetting everything\n");
        // had troubles loading a filename
        base_filename[0] = 'N';
        base_filename[1] = 'O';
        base_filename[2] = 'N';
        base_filename[3] = 'E';
        base_filename[4] = 0;

        // need to reset everything
        anthem_reset();
        verse_reset();
        instrument_reset();
    }
    else // there was a filename to look into
    {
        if (io_load_anthem())
        {
            anthem_reset();
        }
        if (io_load_verse(16))
        {
            verse_reset();
        }
        if (io_load_instrument(16))
        {
            instrument_reset();
        }
    }

    // init game mode
    old_visual_mode = None;
    previous_visual_mode = None;
    game_switch(MainMenu);
}

void game_frame()
{
    kbd_emulate_gamepad();
    switch (visual_mode)
    {
    case MainMenu:
        menu_controls();
        break;
    case GameOn:
        run_controls();
        break;
    case ChooseFilename:
        name_controls();
        break;
    case EditAnthem:
        anthem_controls();
        break;
    case EditVerse:
        verse_controls();
        break;
    case EditInstrument:
        instrument_controls();
        break;
    default:
        if (GAMEPAD_PRESS(0, select))
            game_switch(MainMenu);
        break;
    }
    
    old_gamepad[0] = gamepad_buttons[0];
    old_gamepad[1] = gamepad_buttons[1];
}

void graph_frame() 
{
}

void graph_line() 
{
    if (vga_odd)
        return;
    switch (visual_mode)
    {
        case MainMenu:
            menu_line();
            break;
        case GameOn:
            run_line();
            break;
        case ChooseFilename:
            name_line();
            break;
        case EditAnthem:
            anthem_line();
            break;
        case EditVerse:
            verse_line();
            break;
        case EditInstrument:
            instrument_line();
            break;
        default:
        {
            int line = vga_line/10;
            int internal_line = vga_line%10;
            if (vga_line/2 == 0 || (internal_line/2 == 4))
            {
                memset(draw_buffer, BSOD, 2*SCREEN_W);
                return;
            }
            if (line >= 4 && line < 20)
            {
                line -= 4;
                uint32_t *dst = (uint32_t *)draw_buffer + 37;
                uint32_t color_choice[2] = { (BSOD*257)|((BSOD*257)<<16), 65535|(65535<<16) };
                int shift = ((internal_line/2))*4;
                for (int c=0; c<16; ++c)
                {
                    uint8_t row = (font[c+line*16] >> shift) & 15;
                    for (int j=0; j<4; ++j)
                    {
                        *(++dst) = color_choice[row&1];
                        row >>= 1;
                    }
                    *(++dst) = color_choice[0];
                }
                return;
            }
            break;
        }
    }
}

void game_switch(VisualMode new_visual_mode)
{
    chip_kill();
    game_message[0] = 0;

    switch (new_visual_mode)
    {
    case GameOn:
        run_switch();
        break;
    default:
        break;
    }
    visual_mode = new_visual_mode;
}
