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
#include "bui_font.h"
#include "bui_menu.h"
#include "bui_room.h"

#include "app.h"
#include "app_seedutils.h"

#define APP_ROOM_COMPARESEED_ARGS ((char*) app_room_ctx.frame_ptr)
#define APP_ROOM_COMPARESEED_ACTIVE (*((app_room_compareseed_active_t*) app_room_ctx.stack_ptr - 1))
#define APP_ROOM_COMPARESEED_RET (*((app_room_compareseed_ret_t*) app_room_ctx.frame_ptr))

//----------------------------------------------------------------------------//
//                                                                            //
//                  Internal Type Declarations & Definitions                  //
//                                                                            //
//----------------------------------------------------------------------------//

typedef struct __attribute__((aligned(1))) {
	bool displayed;
} app_room_compareseed_active_t;

//----------------------------------------------------------------------------//
//                                                                            //
//                Internal Variable Declarations & Definitions                //
//                                                                            //
//----------------------------------------------------------------------------//

static const uint8_t app_room_compareseed_bmp_thinking_bb[] = {
	0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0xFF, 0x80,
	0x00, 0x00, 0xFF, 0xC0, 0x00, 0x00, 0xFF, 0xC0,
	0x00, 0x0F, 0xFF, 0xE0, 0x00, 0x30, 0xFF, 0xE0,
	0x00, 0xC1, 0xFF, 0xE0, 0x01, 0x01, 0xFF, 0xE0,
	0x02, 0x03, 0xFF, 0xE0, 0x04, 0x0F, 0xC7, 0xE0,
	0x08, 0x0F, 0x07, 0x90, 0x08, 0x00, 0x0F, 0x10,
	0x10, 0x00, 0x0E, 0x08, 0x10, 0x08, 0x0E, 0x08,
	0x20, 0x0E, 0x06, 0x04, 0x20, 0x03, 0xE6, 0x04,
	0x20, 0x00, 0x00, 0x04, 0x20, 0x00, 0x00, 0x04,
	0x20, 0x00, 0x00, 0x04, 0x20, 0x60, 0x00, 0x04,
	0x20, 0x70, 0x18, 0x04, 0x20, 0x70, 0x1C, 0x04,
	0x10, 0x70, 0x1C, 0x08, 0x11, 0x00, 0x1C, 0x08,
	0x09, 0xF8, 0x18, 0x10, 0x08, 0x00, 0x00, 0x10,
	0x04, 0x00, 0x42, 0x20, 0x02, 0x00, 0x3C, 0x40,
	0x01, 0x00, 0x00, 0x80, 0x00, 0xC0, 0x03, 0x00,
	0x00, 0x30, 0x0C, 0x00, 0x00, 0x0F, 0xF0, 0x00,
};
static const uint32_t app_room_compareseed_bmp_thinking_plt[] = {
	BUI_CLR_BLACK,
	BUI_CLR_WHITE,
};
#define APP_ROOM_COMPARESEED_BMP_THINKING \
		((const bui_const_bitmap_t) { \
			.w = 32, \
			.h = 32, \
			.bb = app_room_compareseed_bmp_thinking_bb, \
			.plt = app_room_compareseed_bmp_thinking_plt, \
			.bpp = 1, \
		})

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_compareseed_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event);

static void app_room_compareseed_enter(bool up);
static void app_room_compareseed_draw();
static void app_room_compareseed_displayed();

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Variable Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

const bui_room_t app_rooms_compareseed = {
	.event_handler = app_room_compareseed_handle_event,
};

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_room_compareseed_handle_event(bui_room_ctx_t *ctx, const bui_room_event_t *event) {
	switch (event->id) {
	case BUI_ROOM_EVENT_ENTER: {
		bool up = BUI_ROOM_EVENT_DATA_ENTER(event)->up;
		app_room_compareseed_enter(up);
	} break;
	case BUI_ROOM_EVENT_DRAW: {
		app_room_compareseed_draw();
	} break;
	case BUI_ROOM_EVENT_FORWARD: {
		const bui_event_t *bui_event = BUI_ROOM_EVENT_DATA_FORWARD(event);
		switch (bui_event->id) {
		case BUI_EVENT_DISPLAYED: {
			app_room_compareseed_displayed();
		} break;
		// Other events are acknowledged
		default:
			break;
		}
	} break;
	// Other events are acknowledged
	default:
		break;
	}
}

static void app_room_compareseed_enter(bool up) {
	if (!app_seedutils_valid_checksum(APP_ROOM_COMPARESEED_ARGS, strlen(APP_ROOM_COMPARESEED_ARGS))) {
		bui_room_dealloc_frame(&app_room_ctx);
		bui_room_alloc(&app_room_ctx, sizeof(app_room_compareseed_ret_t));
		APP_ROOM_COMPARESEED_RET = APP_ROOM_COMPARESEED_RET_CHECKSUM;
		bui_room_exit(&app_room_ctx);
		return;
	}
	bui_room_alloc(&app_room_ctx, sizeof(app_room_compareseed_active_t));
	APP_ROOM_COMPARESEED_ACTIVE.displayed = false;
	app_disp_invalidate();
}

static void app_room_compareseed_draw() {
	bui_ctx_draw_bitmap_full(&app_bui_ctx, APP_ROOM_COMPARESEED_BMP_THINKING, 14, 0);
	bui_font_draw_string(&app_bui_ctx, "Thinking...", 53, 16, BUI_DIR_LEFT, bui_font_open_sans_extrabold_11);
}

static void app_room_compareseed_displayed() {
	if (!APP_ROOM_COMPARESEED_ACTIVE.displayed) {
		APP_ROOM_COMPARESEED_ACTIVE.displayed = true;
		app_disp_invalidate();
		return;
	}
	bool equal = app_seedutils_compare(APP_ROOM_COMPARESEED_ARGS, strlen(APP_ROOM_COMPARESEED_ARGS));
	bui_room_dealloc_frame(&app_room_ctx);
	bui_room_alloc(&app_room_ctx, sizeof(app_room_compareseed_ret_t));
	APP_ROOM_COMPARESEED_RET = equal ? APP_ROOM_COMPARESEED_RET_EQUAL : APP_ROOM_COMPARESEED_RET_UNEQUAL;
	bui_room_exit(&app_room_ctx);
}
