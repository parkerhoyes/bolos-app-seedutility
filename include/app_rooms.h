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

#ifndef APP_ROOMS_H_
#define APP_ROOMS_H_

#include <stdint.h>

#include "bui_room.h"

#include "app.h"

//----------------------------------------------------------------------------//
//                                                                            //
//                  External Type Declarations & Definitions                  //
//                                                                            //
//----------------------------------------------------------------------------//

typedef struct {
	// The number of words in the seed; 12, 18, or 24
	uint8_t seed_length;
} app_room_chooselength_ret_t;

typedef struct __attribute__((aligned(4))) {
	// The number of words in the seed; 12, 18, or 24
	uint8_t seed_length;
} app_room_enterseed_args_t;

typedef struct __attribute__((aligned(4))) {
	// The buffer in which to store the word (null-terminator included). Must have a capacity of at least
	// APP_SEEDUTILS_WORD_LEN_MAX + 1 bytes.
	char *word_buff;
} app_room_enterword_args_t;

typedef uint8_t app_room_compareseed_ret_t;
#define APP_ROOM_COMPARESEED_RET_EQUAL    ((app_room_compareseed_ret_t) 0)
#define APP_ROOM_COMPARESEED_RET_UNEQUAL  ((app_room_compareseed_ret_t) 1)
#define APP_ROOM_COMPARESEED_RET_CHECKSUM ((app_room_compareseed_ret_t) 2)

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Variable Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

extern const bui_room_t app_rooms_main;

extern const bui_room_t app_rooms_verifybackup;
extern const bui_room_t app_rooms_chooselength;
extern const bui_room_t app_rooms_enterseed;
extern const bui_room_t app_rooms_enterword;
extern const bui_room_t app_rooms_compareseed;

extern const bui_room_t app_rooms_about;

#endif
