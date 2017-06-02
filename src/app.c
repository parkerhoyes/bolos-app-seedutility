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

#include "app.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "os.h"
#include "os_io_seproxyhal.h"

#include "bui.h"
#include "bui_room.h"

#include "app_rooms.h"

#define APP_TICKER_INTERVAL 40

//----------------------------------------------------------------------------//
//                                                                            //
//                Internal Variable Declarations & Definitions                //
//                                                                            //
//----------------------------------------------------------------------------//

/*
 * Internal Non-const (RAM) Variable Definitions
 */

static uint8_t app_room_ctx_stack[APP_ROOM_CTX_STACK_SIZE] __attribute__((aligned(4)));
static bool app_disp_invalidated; // true if the display needs to be redrawn

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_handle_bui_event(bui_ctx_t *ctx, const bui_event_t *event);

static void app_display();

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Variable Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

/*
 * External Non-const (RAM) Variable Definitions
 */

bui_ctx_t app_bui_ctx;
bui_room_ctx_t app_room_ctx;

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Function Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

void app_init() {
	// Initialize global vars
	app_disp_invalidated = true;
	bui_ctx_init(&app_bui_ctx);
	bui_ctx_set_event_handler(&app_bui_ctx, app_handle_bui_event);
	bui_ctx_set_ticker(&app_bui_ctx, APP_TICKER_INTERVAL);

	// Launch the GUI
	bui_room_ctx_init(&app_room_ctx, app_room_ctx_stack, &app_rooms_main, NULL, 0);

	// Draw the first frame
	app_display();
}

void app_io_event() {
	// Pass the event on to BUI for handling
	bui_ctx_seproxyhal_event(&app_bui_ctx, true);
}

void app_disp_invalidate() {
	app_disp_invalidated = true;
}

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

static void app_handle_bui_event(bui_ctx_t *ctx, const bui_event_t *event) {
	bui_room_forward_event(&app_room_ctx, event);
	switch (event->id) {
	case BUI_EVENT_TIME_ELAPSED: {
		if (app_disp_invalidated && bui_ctx_is_displayed(&app_bui_ctx)) {
			app_display();
			app_disp_invalidated = false;
		}
	} break;
	// Other events are acknowledged
	default:
		break;
	}
}

static void app_display() {
	bui_ctx_fill(&app_bui_ctx, BUI_CLR_BLACK);
	// Draw the current room by dispatching event BUI_ROOM_EVENT_DRAW
	{
		bui_room_event_data_draw_t data = { .bui_ctx = &app_bui_ctx };
		bui_room_event_t event = { .id = BUI_ROOM_EVENT_DRAW, .data = &data };
		bui_room_dispatch_event(&app_room_ctx, &event);
	}
	bui_ctx_display(&app_bui_ctx);
}
