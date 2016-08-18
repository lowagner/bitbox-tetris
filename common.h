#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 240

#define HOLE_X 10
#define HOLE_Y 20
#define SQUARE 10  // make divisible by 2

typedef enum {
    None=0,
    MainMenu,
    GameOn,
    EditAnthem,
    EditVerse,
    EditInstrument,
    ChooseFilename
} VisualMode;

extern VisualMode visual_mode;
extern VisualMode previous_visual_mode;

void game_switch(VisualMode new_visual_mode);

#define GAMEPAD_PRESS(id, key) ((gamepad_buttons[id]) & (~old_gamepad[id]) & (gamepad_##key))
#define GAMEPAD_PRESSING(id, key) ((gamepad_buttons[id]) & (gamepad_##key) & (~old_gamepad[id] | ((gamepad_press_wait == 0)*gamepad_##key)))
#define GAMEPAD_PRESS_WAIT 8
extern uint8_t gamepad_press_wait;
extern uint16_t old_gamepad[2];

extern uint8_t game_message[32];
extern const uint8_t hex[64]; // not exactly hex but ok!

// various game parameters
extern uint8_t game_players;
extern uint8_t game_wide;
extern uint8_t game_fall_speed;
extern uint8_t game_raise_speed;
extern uint8_t game_start_height;
extern uint8_t game_torus;

extern uint64_t score;
extern uint32_t scores[2];
extern uint32_t top_scores[2];
extern uint64_t top_coop_score;
extern uint64_t top_wide_score;

// 4-square polyomino types
#define O4 0
#define I4 1
#define T4 2
#define L4 3
#define J4 4
#define S4 5
#define Z4 6

uint8_t *write_hex(uint8_t *c, uint64_t number);

#endif
