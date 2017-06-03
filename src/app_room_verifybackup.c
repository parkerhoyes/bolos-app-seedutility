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

#include "bui.h"
#include "bui_room.h"

#include "app.h"

/*
 * Room Memory Management Strategy:
 *
 * This room always has app_room_verifybackup_data_t allocated at the bottom of its stack frame, and then when the
 * app_rooms_enterseed room is called, the mnemonic is pushed onto the stack as well.
 */

#define APP_ROOM_VERIFYBACKUP_DATA (*((app_room_verifybackup_data_t*) app_room_ctx.frame_ptr))
#define APP_ROOM_VERIFYBACKUP_CHOOSELENGTH_RET (*((app_room_chooselength_ret_t*) (&APP_ROOM_VERIFYBACKUP_DATA + 1)))
#define APP_ROOM_VERIFYBACKUP_ENTERSEED_RET ((char*) (&APP_ROOM_VERIFYBACKUP_DATA + 1))

//----------------------------------------------------------------------------//
//                                                                            //
//                  Internal Type Declarations & Definitions                  //
//                                                                            //
//----------------------------------------------------------------------------//

typedef uint8_t app_room_verifybackup_state_t;

#define APP_ROOM_VERIFYBACKUP_STATE_CHOOSELENGTH ((app_room_verifybackup_state_t) 0)
#define APP_ROOM_VERIFYBACKUP_STATE_ENTERSEED    ((app_room_verifybackup_state_t) 1)
#define APP_ROOM_VERIFYBACKUP_STATE_COMPARESEED  ((app_room_verifybackup_state_t) 2)
#define APP_ROOM_VERIFYBACKUP_STATE_RESULTS      ((app_room_verifybackup_state_t) 3)
#define APP_ROOM_VERIFYBACKUP_STATE_DONE         ((app_room_verifybackup_state_t) 4)

typedef struct __attribute__((aligned(4))) {
	// State of the room
	app_room_verifybackup_state_t state;
} app_room_verifybackup_data_t;

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_verifybackup_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event);

static void app_room_verifybackup_enter(bool up);
static void app_room_verifybackup_exit(bool up);

static void app_room_verifybackup_advance();

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Variable Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

const bui_room_t app_rooms_verifybackup = {
	.event_handler = app_room_verifybackup_handle_event,
};

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_verifybackup_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event) {
	switch (event->id) {
	case BUI_ROOM_EVENT_ENTER: {
		bool up = BUI_ROOM_EVENT_DATA_ENTER(event)->up;
		app_room_verifybackup_enter(up);
	} break;
	case BUI_ROOM_EVENT_EXIT: {
		bool up = BUI_ROOM_EVENT_DATA_EXIT(event)->up;
		app_room_verifybackup_exit(up);
	} break;
	// Other events are acknowledged
	default:
		break;
	}
}

static void app_room_verifybackup_enter(bool up) {
	if (up) {
		bui_room_alloc(&app_room_ctx, sizeof(app_room_verifybackup_data_t));
		APP_ROOM_VERIFYBACKUP_DATA.state = APP_ROOM_VERIFYBACKUP_STATE_CHOOSELENGTH;
	}
	app_room_verifybackup_advance();
}

static void app_room_verifybackup_exit(bool up) {
	if (!up)
		bui_room_dealloc_frame(&app_room_ctx);
}

static void app_room_verifybackup_advance() {
	switch (APP_ROOM_VERIFYBACKUP_DATA.state) {
	case APP_ROOM_VERIFYBACKUP_STATE_CHOOSELENGTH: {
		APP_ROOM_VERIFYBACKUP_DATA.state = APP_ROOM_VERIFYBACKUP_STATE_ENTERSEED;
		bui_room_enter(&app_room_ctx, &app_rooms_chooselength, NULL, 0);
	} break;
	case APP_ROOM_VERIFYBACKUP_STATE_ENTERSEED: {
		APP_ROOM_VERIFYBACKUP_DATA.state = APP_ROOM_VERIFYBACKUP_STATE_COMPARESEED;
		uint8_t seed_length = APP_ROOM_VERIFYBACKUP_CHOOSELENGTH_RET.seed_length;
		bui_room_dealloc(&app_room_ctx, sizeof(app_room_chooselength_ret_t));
		app_room_enterseed_args_t args = { .seed_length = seed_length };
		bui_room_enter(&app_room_ctx, &app_rooms_enterseed, &args, sizeof(args));
	} break;
	case APP_ROOM_VERIFYBACKUP_STATE_COMPARESEED: {
		APP_ROOM_VERIFYBACKUP_DATA.state = APP_ROOM_VERIFYBACKUP_STATE_RESULTS;
		uint8_t mnemonic_len = strlen(APP_ROOM_VERIFYBACKUP_ENTERSEED_RET);
		app_disp_invalidate();
		bui_room_enter(&app_room_ctx, &app_rooms_compareseed, NULL, mnemonic_len + 1);
	} break;
	case APP_ROOM_VERIFYBACKUP_STATE_RESULTS: {
		APP_ROOM_VERIFYBACKUP_DATA.state = APP_ROOM_VERIFYBACKUP_STATE_DONE;
		app_room_compareseed_ret_t compareseed_ret;
		bui_room_pop(&app_room_ctx, &compareseed_ret, sizeof(compareseed_ret));
		const char *msg;
		switch (compareseed_ret) {
		case APP_ROOM_COMPARESEED_RET_EQUAL:
			msg = "The seed you entered\nis the same as the\nseed on the device.";
			break;
		case APP_ROOM_COMPARESEED_RET_UNEQUAL:
			msg = "The seed you entered\nis NOT the same as the\nseed on the device.";
			break;
		case APP_ROOM_COMPARESEED_RET_CHECKSUM:
			msg = "The seed you entered\nis an invalid BIP 39\nseed (checksum).";
			break;
		}
		bui_room_message_args_t message_args = { .msg = msg, .font = bui_font_lucida_console_8 };
		app_disp_invalidate();
		bui_room_enter(&app_room_ctx, &bui_room_message, &message_args, sizeof(message_args));
	} break;
	case APP_ROOM_VERIFYBACKUP_STATE_DONE: {
		bui_room_exit(&app_room_ctx);
	} break;
	}
}
