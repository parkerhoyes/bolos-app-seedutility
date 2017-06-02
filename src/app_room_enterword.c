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
#include <string.h>

#include "os.h"

#include "bui.h"
#include "bui_bkb.h"
#include "bui_room.h"

#include "app.h"
#include "app_seedutils.h"

#define APP_ROOM_ENTERWORD_ARGS (*((app_room_enterword_args_t*) app_room_ctx.frame_ptr))
#define APP_ROOM_ENTERWORD_ACTIVE (*((app_room_enterword_active_t*) (&APP_ROOM_ENTERWORD_ARGS + 1)))
#define APP_ROOM_ENTERWORD_INACTIVE (*((app_room_enterword_inactive_t*) (&APP_ROOM_ENTERWORD_ARGS + 1)))

//----------------------------------------------------------------------------//
//                                                                            //
//                  Internal Type Declarations & Definitions                  //
//                                                                            //
//----------------------------------------------------------------------------//

typedef struct {
	bui_bkb_bkb_t bkb;
	bool ready;
} app_room_enterword_active_t;

// NOTE: This room is only inactive when confirming a word prediction.
typedef struct {
	// The index of the word being predicted
	uint16_t word_index;
	// The predicted word as a null-terminated string
	char word[APP_SEEDUTILS_WORD_LEN_MAX + 1];
} app_room_enterword_inactive_t;

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_enterword_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event);

static void app_room_enterword_enter(bool up);
static void app_room_enterword_exit(bool up);
static void app_room_enterword_draw();
static void app_room_enterword_time_elapsed(uint32_t elapsed);
static void app_room_enterword_button_clicked(bui_button_id_t button);

static void app_room_enterword_update_bkb();

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Variable Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

const bui_room_t app_rooms_enterword = {
	.event_handler = app_room_enterword_handle_event,
};

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_enterword_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event) {
	switch (event->id) {
	case BUI_ROOM_EVENT_ENTER: {
		bool up = BUI_ROOM_EVENT_DATA_ENTER(event)->up;
		app_room_enterword_enter(up);
	} break;
	case BUI_ROOM_EVENT_EXIT: {
		bool up = BUI_ROOM_EVENT_DATA_EXIT(event)->up;
		app_room_enterword_exit(up);
	} break;
	case BUI_ROOM_EVENT_DRAW: {
		app_room_enterword_draw();
	} break;
	case BUI_ROOM_EVENT_FORWARD: {
		const bui_event_t *bui_event = BUI_ROOM_EVENT_DATA_FORWARD(event);
		switch (bui_event->id) {
		case BUI_EVENT_TIME_ELAPSED: {
			uint32_t elapsed = BUI_EVENT_DATA_TIME_ELAPSED(bui_event)->elapsed;
			app_room_enterword_time_elapsed(elapsed);
		} break;
		case BUI_EVENT_BUTTON_CLICKED: {
			bui_button_id_t button = BUI_EVENT_DATA_BUTTON_CLICKED(bui_event)->button;
			app_room_enterword_button_clicked(button);
		} break;
		// Other events are acknowledged
		default:
			break;
		}
	} break;
	}
}

static void app_room_enterword_enter(bool up) {
	if (!up) {
		bui_room_confirm_ret_t confirm_ret;
		bui_room_pop(&app_room_ctx, &confirm_ret, sizeof(confirm_ret));
		if (confirm_ret.confirmed) {
			uint8_t word_len;
			const char *word = app_seedutils_bip39_word(APP_ROOM_ENTERWORD_INACTIVE.word_index, &word_len);
			os_memcpy(APP_ROOM_ENTERWORD_ARGS.word_buff, word, word_len);
			APP_ROOM_ENTERWORD_ARGS.word_buff[word_len] = '\0';
			bui_room_exit(&app_room_ctx);
			return;
		}
		bui_room_dealloc(&app_room_ctx, sizeof(app_room_enterword_inactive_t));
	}
	bui_room_alloc(&app_room_ctx, sizeof(app_room_enterword_active_t));
	bui_bkb_init(&APP_ROOM_ENTERWORD_ACTIVE.bkb, NULL, 0, APP_ROOM_ENTERWORD_ARGS.word_buff, 0,
			APP_SEEDUTILS_WORD_LEN_MAX, true);
	app_room_enterword_update_bkb();
	app_disp_invalidate();
}

static void app_room_enterword_exit(bool up) {
	// Memory management on exit when up == true is done at room call location
	if (!up)
		bui_room_dealloc_frame(&app_room_ctx);
}

static void app_room_enterword_draw() {
	bui_bkb_draw(&APP_ROOM_ENTERWORD_ACTIVE.bkb, &app_bui_ctx);
}

static void app_room_enterword_time_elapsed(uint32_t elapsed) {
	if (bui_bkb_animate(&APP_ROOM_ENTERWORD_ACTIVE.bkb, elapsed))
		app_disp_invalidate();
}

static void app_room_enterword_button_clicked(bui_button_id_t button) {
	bui_dir_t dir;
	switch (button) {
	case BUI_BUTTON_NANOS_BOTH:
		if (APP_ROOM_ENTERWORD_ACTIVE.ready) {
			uint8_t type_buff_size = bui_bkb_get_type_buff_size(&APP_ROOM_ENTERWORD_ACTIVE.bkb);
			APP_ROOM_ENTERWORD_ARGS.word_buff[type_buff_size] = '\0';
			bui_room_exit(&app_room_ctx);
		}
		return;
	case BUI_BUTTON_NANOS_LEFT:
		dir = BUI_DIR_LEFT;
		break;
	case BUI_BUTTON_NANOS_RIGHT:
		dir = BUI_DIR_RIGHT;
		break;
	}
	// Control only reaches here if the left or right button was clicked (not both)
	int choice = bui_bkb_choose(&APP_ROOM_ENTERWORD_ACTIVE.bkb, dir);
	if (choice <= 0xFF || choice == 0x2FF)
		app_room_enterword_update_bkb();
	app_disp_invalidate();
}

static void app_room_enterword_update_bkb() {
	uint8_t type_buff_size = bui_bkb_get_type_buff_size(&APP_ROOM_ENTERWORD_ACTIVE.bkb);
	char *layout = bui_room_alloc(&app_room_ctx, 26);
	bool complete;
	int16_t prediction;
	uint8_t layout_size = app_seedutils_bip39_next_letters(APP_ROOM_ENTERWORD_ARGS.word_buff, type_buff_size, layout,
			&complete, &prediction);
	if (prediction != -1) {
		bui_room_dealloc(&app_room_ctx, 26);
		uint8_t word_len;
		const char *word = app_seedutils_bip39_word(prediction, &word_len);
		bui_room_dealloc(&app_room_ctx, sizeof(app_room_enterword_active_t));
		bui_room_alloc(&app_room_ctx, sizeof(app_room_enterword_inactive_t));
		APP_ROOM_ENTERWORD_INACTIVE.word_index = prediction;
		os_memcpy(APP_ROOM_ENTERWORD_INACTIVE.word, word, word_len);
		APP_ROOM_ENTERWORD_INACTIVE.word[word_len] = '\0';
		bui_room_confirm_args_t args = {
			.msg = APP_ROOM_ENTERWORD_INACTIVE.word,
			.font = bui_font_open_sans_extrabold_11,
		};
		app_disp_invalidate();
		bui_room_enter(&app_room_ctx, &bui_room_confirm, &args, sizeof(args));
		return;
	}
	APP_ROOM_ENTERWORD_ACTIVE.ready = type_buff_size == 0 || complete;
	bui_bkb_set_layout(&APP_ROOM_ENTERWORD_ACTIVE.bkb, layout, layout_size);
	bui_room_dealloc(&app_room_ctx, 26);
}
