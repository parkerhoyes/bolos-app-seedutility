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

#ifndef APP_H_
#define APP_H_

#include <stdbool.h>
#include <stdint.h>

#include "os.h"

#include "bui.h"
#include "bui_room.h"

#define APP_VER_MAJOR 1
#define APP_VER_MINOR 0
#define APP_VER_PATCH 0

// This should be enough ¯\_(ツ)_/¯
#define APP_ROOM_CTX_STACK_SIZE 1024

#define APP_STR(x) APP_STR_(x)
#define APP_STR_(x) #x

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Variable Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

/*
 * External Non-const (RAM) Variable Declarations
 */

extern bui_ctx_t app_bui_ctx;
extern bui_room_ctx_t app_room_ctx;

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Function Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

void app_init();
void app_io_event();
void app_disp_invalidate();

#endif
