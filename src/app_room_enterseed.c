/*
 * License for the BOLOS Seed Utility Application project, originally found
 * here: https://github.com/parkerhoyes/bolos-app-seedutility
 *
 * Copyright (C) 2017 Parker Hoyes <contact@parkerhoyes.com>
 *
 * This software is provided "as-is", without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim
 *    that you wrote the original software. If you use this software in a
 *    product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "app_rooms.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "os.h"

#include "bui.h"
#include "bui_font.h"
#include "bui_menu.h"
#include "bui_room.h"

#include "app.h"
#include "app_seedutils.h"

/*
 * Room Memory Management Strategy:
 *
 * This room always has app_room_enterseed_args_t allocated at the bottom of its stack frame, followed by seed_length
 * copies of app_room_enterseed_word_t. Then, it has either app_room_enterseed_active_t or app_room_enterseed_inactive_t
 * allocated at the top of its stack frame, depending on whether or not it is the current room.
 */

#define APP_ROOM_ENTERSEED_ARGS (*((app_room_enterseed_args_t*) app_room_ctx.frame_ptr))
#define APP_ROOM_ENTERSEED_WORDS ((char*) (&APP_ROOM_ENTERSEED_ARGS + 1))
#define APP_ROOM_ENTERSEED_ACTIVE (*((app_room_enterseed_active_t*) (APP_ROOM_ENTERSEED_WORDS + \
		APP_ROOM_ENTERSEED_WORDS_LEN)))

#define APP_ROOM_ENTERSEED_MENU_SIZE (APP_ROOM_ENTERSEED_ARGS.seed_length + 2)
#define APP_ROOM_ENTERSEED_WORD_LEN (APP_SEEDUTILS_WORD_LEN_MAX + 1)
#define APP_ROOM_ENTERSEED_WORDS_LEN (APP_ROOM_ENTERSEED_WORD_LEN * APP_ROOM_ENTERSEED_ARGS.seed_length)

//----------------------------------------------------------------------------//
//                                                                            //
//                  Internal Type Declarations & Definitions                  //
//                                                                            //
//----------------------------------------------------------------------------//

typedef struct {
	bui_menu_menu_t menu;
} app_room_enterseed_active_t;

typedef struct {
	// The index of the focused menu element
	uint8_t focus;
} app_room_enterseed_inactive_t;

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_enterseed_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event);

static void app_room_enterseed_enter(bool up);
static void app_room_enterseed_exit(bool up);
static void app_room_enterseed_draw();
static void app_room_enterseed_time_elapsed(uint32_t elapsed);
static void app_room_enterseed_button_clicked(bui_button_id_t button);

static uint8_t app_room_enterseed_elem_size(const bui_menu_menu_t *menu, uint8_t i);
static void app_room_enterseed_elem_draw(const bui_menu_menu_t *menu, uint8_t i, bui_ctx_t *bui_ctx, int16_t y);

static bool app_room_enterseed_everything_entered();

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Variable Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

const bui_room_t app_rooms_enterseed = {
	.event_handler = app_room_enterseed_handle_event,
};

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_enterseed_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event) {
	switch (event->id) {
	case BUI_ROOM_EVENT_ENTER: {
		bool up = BUI_ROOM_EVENT_DATA_ENTER(event)->up;
		app_room_enterseed_enter(up);
	} break;
	case BUI_ROOM_EVENT_EXIT: {
		bool up = BUI_ROOM_EVENT_DATA_EXIT(event)->up;
		app_room_enterseed_exit(up);
	} break;
	case BUI_ROOM_EVENT_DRAW: {
		app_room_enterseed_draw();
	} break;
	case BUI_ROOM_EVENT_FORWARD: {
		const bui_event_t *bui_event = BUI_ROOM_EVENT_DATA_FORWARD(event);
		switch (bui_event->id) {
		case BUI_EVENT_TIME_ELAPSED: {
			uint32_t elapsed = BUI_EVENT_DATA_TIME_ELAPSED(bui_event)->elapsed;
			app_room_enterseed_time_elapsed(elapsed);
		} break;
		case BUI_EVENT_BUTTON_CLICKED: {
			bui_button_id_t button = BUI_EVENT_DATA_BUTTON_CLICKED(bui_event)->button;
			app_room_enterseed_button_clicked(button);
		} break;
		// Other events are acknowledged
		default:
			break;
		}
	} break;
	}
}

static void app_room_enterseed_enter(bool up) {
	uint8_t focus;
	if (up) {
		bui_room_alloc(&app_room_ctx, APP_ROOM_ENTERSEED_WORDS_LEN);
		os_memset(APP_ROOM_ENTERSEED_WORDS, 0, APP_ROOM_ENTERSEED_WORDS_LEN);
		focus = 0;
	} else {
		app_room_enterseed_inactive_t inactive;
		bui_room_pop(&app_room_ctx, &inactive, sizeof(inactive));
		focus = inactive.focus;
	}
	bui_room_alloc(&app_room_ctx, sizeof(app_room_enterseed_active_t));
	APP_ROOM_ENTERSEED_ACTIVE.menu.elem_size_callback = app_room_enterseed_elem_size;
	APP_ROOM_ENTERSEED_ACTIVE.menu.elem_draw_callback = app_room_enterseed_elem_draw;
	bui_menu_init(&APP_ROOM_ENTERSEED_ACTIVE.menu, APP_ROOM_ENTERSEED_MENU_SIZE, focus, true);
	app_disp_invalidate();
}

static void app_room_enterseed_exit(bool up) {
	if (up) {
		app_room_enterseed_inactive_t inactive;
		inactive.focus = bui_menu_get_focused(&APP_ROOM_ENTERSEED_ACTIVE.menu);
		bui_room_dealloc(&app_room_ctx, sizeof(app_room_enterseed_active_t));
		bui_room_push(&app_room_ctx, &inactive, sizeof(inactive));
	} else {
		// Collapse all words into the mnemonic (remove zero padding), and deallocate everything else on the stack
		uint8_t seed_length = APP_ROOM_ENTERSEED_ARGS.seed_length;
		char *words = APP_ROOM_ENTERSEED_WORDS;
		char *mnemonic = app_room_ctx.frame_ptr;
		uint8_t mnemonic_len = 0;
		for (uint8_t i = 0; i < seed_length; i++) {
			uint8_t word_len = 0;
			for (char *ch = &words[APP_ROOM_ENTERSEED_WORD_LEN * i]; *ch != '\0'; ch++)
				word_len++;
			os_memmove(&mnemonic[mnemonic_len], &words[APP_ROOM_ENTERSEED_WORD_LEN * i], word_len);
			mnemonic_len += word_len;
			mnemonic[mnemonic_len++] = i + 1 != seed_length ? ' ' : '\0';
		}
		app_room_ctx.stack_ptr = (char*) app_room_ctx.frame_ptr + mnemonic_len;
	}
}

static void app_room_enterseed_draw() {
	bui_menu_draw(&APP_ROOM_ENTERSEED_ACTIVE.menu, &app_bui_ctx);
}

static void app_room_enterseed_time_elapsed(uint32_t elapsed) {
	if (bui_menu_animate(&APP_ROOM_ENTERSEED_ACTIVE.menu, elapsed))
		app_disp_invalidate();
}

static void app_room_enterseed_button_clicked(bui_button_id_t button) {
	switch (button) {
	case BUI_BUTTON_NANOS_BOTH: {
		uint8_t focused = bui_menu_get_focused(&APP_ROOM_ENTERSEED_ACTIVE.menu);
		if (focused == 0) { // "Type in your seed" prompt was selected
			// Do nothing
		} else if (focused == APP_ROOM_ENTERSEED_MENU_SIZE - 1) { // "Done" was selected
			if (app_room_enterseed_everything_entered()) {
				bui_room_exit(&app_room_ctx);
			} else {
				bui_room_message_args_t args = {
					.msg = "Not all words\nhave been entered.",
					.font = bui_font_open_sans_extrabold_11,
				};
				app_disp_invalidate();
				bui_room_enter(&app_room_ctx, &bui_room_message, &args, sizeof(args));
			}
		} else { // A word was selected
			app_room_enterword_args_t args = {
				.word_buff = &APP_ROOM_ENTERSEED_WORDS[APP_ROOM_ENTERSEED_WORD_LEN * (focused - 1)],
			};
			bui_room_enter(&app_room_ctx, &app_rooms_enterword, &args, sizeof(args));
		}
	} break;
	case BUI_BUTTON_NANOS_LEFT:
		bui_menu_scroll(&APP_ROOM_ENTERSEED_ACTIVE.menu, true);
		app_disp_invalidate();
		break;
	case BUI_BUTTON_NANOS_RIGHT:
		bui_menu_scroll(&APP_ROOM_ENTERSEED_ACTIVE.menu, false);
		app_disp_invalidate();
		break;
	}
}

static uint8_t app_room_enterseed_elem_size(const bui_menu_menu_t *menu, uint8_t i) {
	if (i == 0)
		return 27;
	else if (i == APP_ROOM_ENTERSEED_MENU_SIZE - 1)
		return 15;
	else
		return 24;
}

static void app_room_enterseed_elem_draw(const bui_menu_menu_t *menu, uint8_t i, bui_ctx_t *bui_ctx, int16_t y) {
	if (i == 0) {
		bui_font_draw_string(&app_bui_ctx, "Type in your", 64, y + 2, BUI_DIR_TOP, bui_font_open_sans_extrabold_11);
		bui_font_draw_string(&app_bui_ctx, "seed below:", 64, y + 14, BUI_DIR_TOP, bui_font_open_sans_extrabold_11);
	} else if (i == APP_ROOM_ENTERSEED_MENU_SIZE - 1) {
		bui_font_draw_string(&app_bui_ctx, "Done", 64, y + 2, BUI_DIR_TOP, bui_font_open_sans_extrabold_11);
	} else {
		{
			char text[9];
			snprintf(text, sizeof(text), "Word #%u", i);
			bui_font_draw_string(&app_bui_ctx, text, 64, y + 2, BUI_DIR_TOP, bui_font_lucida_console_8);
		}
		{
			const char *text = &APP_ROOM_ENTERSEED_WORDS[APP_ROOM_ENTERSEED_WORD_LEN * (i - 1)];
			if (text[0] == '\0')
				text = "Select to enter";
			bui_font_draw_string(&app_bui_ctx, text, 64, y + 11, BUI_DIR_TOP, bui_font_open_sans_extrabold_11);
		}
	}
}

static bool app_room_enterseed_everything_entered() {
	for (uint8_t i = 0; i < APP_ROOM_ENTERSEED_ARGS.seed_length; i++) {
		if (APP_ROOM_ENTERSEED_WORDS[APP_ROOM_ENTERSEED_WORD_LEN * i] == '\0')
			return false;
	}
	return true;
}
