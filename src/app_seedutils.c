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

#include "app_seedutils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"

//----------------------------------------------------------------------------//
//                                                                            //
//                Internal Variable Declarations & Definitions                //
//                                                                            //
//----------------------------------------------------------------------------//

#include "app_seedutils_bip39_data.inc"

// The BIP 32 path node index 6516080'
static const uint32_t app_seedutils_compare_path = 0x80636D70;

// Order of the secp256k1 curve, big-endian
static const uint8_t app_seedutils_secp256k1_order[32] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
	0xBA, 0xAE, 0xDC, 0xE6, 0xAF, 0x48, 0xA0, 0x3B,
	0xBF, 0xD2, 0x5E, 0x8C, 0xD0, 0x36, 0x41, 0x41,
};

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Declarations                       //
//                                                                            //
//----------------------------------------------------------------------------//

/*
 * Bitwise-OR an 11-bit sequence onto the specified bit array.
 *
 * Args:
 *     arr: the bit array
 *     i: the index of the 11-bit destination sequence
 *     n: the number to OR onto the destination; must be in [0, 2047]
 */
void app_seedutils_set_uint11(uint8_t *arr, uint8_t i, uint16_t n);

//----------------------------------------------------------------------------//
//                                                                            //
//                       External Function Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

uint8_t app_seedutils_bip39_next_letters(const char *word, uint8_t word_len, char *letters_dest, bool *complete,
		int16_t *prediction) {
	if (word_len == 0) {
		if (complete != NULL)
			*complete = false;
		if (prediction != NULL)
			*prediction = -1;
		os_memcpy(letters_dest, "abcdefghijklmnopqrstuvwyz", 25);
		return 25;
	}
	if (word_len >= APP_SEEDUTILS_WORD_LEN_MAX) {
		if (complete != NULL)
			*complete = false;
		if (prediction != NULL)
			*prediction = -1;
		return 0;
	}
	uint32_t letters_found = 0; // Bit array representing letters found, in alpha order starting at most significant bit
	bool comp = false;
	uint16_t valid_words = 0; // Number of words for which word is a prefix
	uint16_t valid_wordi; // Index of the last word for which word is a prefix
	for (uint16_t wordi = 0; wordi < APP_SEEDUTILS_WORD_COUNT; wordi++) {
		uint16_t offset = app_seedutils_bip39_wordlist_offsets[wordi];
		uint16_t len = app_seedutils_bip39_wordlist_offsets[wordi + 1] - offset;
		if (len < word_len || (comp && len == word_len))
			goto next_word;
		const char *w = &app_seedutils_bip39_wordlist_words[offset];
		for (uint8_t i = 0; i < word_len; i++) {
			if (w[i] != word[i])
				goto next_word;
		}
		valid_words++;
		valid_wordi = wordi;
		if (len == word_len) {
			comp = true;
			goto next_word;
		}
		char next = w[word_len];
		letters_found |= ((uint32_t) 1 << 25) >> (next - 'a');
	next_word:
		continue;
	}
	if (complete != NULL)
		*complete = comp;
	if (prediction != NULL)
		*prediction = valid_words == 1 ? valid_wordi : -1;
	uint8_t n_found = 0;
	for (uint8_t i = 0; i < 26; i++) {
		if (((letters_found << i) & ((uint32_t) 1 << 25)) != 0) {
			n_found++;
			*letters_dest++ = 'a' + i;
		}
	}
	return n_found;
}

const char* app_seedutils_bip39_word(uint16_t index, uint8_t *word_len_dest) {
	uint16_t offset = app_seedutils_bip39_wordlist_offsets[index];
	if (word_len_dest != NULL)
		*word_len_dest = app_seedutils_bip39_wordlist_offsets[index + 1] - offset;
	return &app_seedutils_bip39_wordlist_words[offset];
}

uint16_t app_seedutils_bip39_index(const char *word, uint8_t word_len) {
	// TODO This could be heavily optimized by performing a binary search instead
	if (word_len < APP_SEEDUTILS_WORD_LEN_MIN || word_len > APP_SEEDUTILS_WORD_LEN_MAX)
		goto not_found;
	uint16_t wordi = 0;
	uint16_t chari = 0;
	for (; wordi < APP_SEEDUTILS_WORD_COUNT; wordi++) {
		uint16_t offset = app_seedutils_bip39_wordlist_offsets[wordi];
		uint16_t len = app_seedutils_bip39_wordlist_offsets[wordi + 1] - offset;
		if (len != word_len)
			goto next_word;
		const char *w = &app_seedutils_bip39_wordlist_words[offset];
		for (uint8_t i = 0; i < len; i++) {
			if (w[i] != word[i])
				goto next_word;
		}
		return wordi;
	next_word:
		chari += len;
	}
not_found:
	return APP_SEEDUTILS_WORD_COUNT;
}

bool app_seedutils_compare(char *mnemonic, uint8_t mnemonic_len) {
	// This will eventually store the master private key at arg_node[0:32] and the master chain code at arg_node[32:64],
	// both of which are derived from the argument mnemonic.
	uint8_t arg_node[64];
	// Generate the seed data according to BIP 39, and store it in arg_node[0:64]. This will be used as the 512 bits of
	// entropy from which the BIP 32 master node is derived.
	if (mnemonic_len > 128) {
		cx_hash_sha512((unsigned char*) mnemonic, mnemonic_len, (unsigned char*) mnemonic);
		mnemonic_len = 64;
	}
	{
		uint8_t passphrase[12];
		os_memcpy(passphrase, "mnemonic\0\0\0", 12);
		cx_pbkdf2_sha512((unsigned char*) mnemonic, mnemonic_len, passphrase, sizeof(passphrase), 2048, arg_node, 64);
	}
	// Calculate the master private key (stored in arg_node[0:32]) and master chain code (stored in arg_node[32:64])
	// according to BIP 32. Both the master private key and master chain code are big-endian 256-bit integers.
	cx_hmac_sha512((unsigned char*) "Bitcoin seed", 12, arg_node, 64, arg_node);
	// Derive the node m / app_seedutils_compare_path (hardened) using the seed derived from the argument mnemonic. The
	// child private key is stored in arg_node[0:32] and the child chain code is stored in arg_node[32:64].
	{
		uint8_t temp[65];
		temp[0] = 0;
		os_memcpy(temp + 1, arg_node, 32);
		while (true) {
			bool failed = false;

			// Store path in temp[33:37], big-endian
			temp[33] = (app_seedutils_compare_path >> 24) & 0xFF;
			temp[34] = (app_seedutils_compare_path >> 16) & 0xFF;
			temp[35] = (app_seedutils_compare_path >> 8) & 0xFF;
			temp[36] = (app_seedutils_compare_path) & 0xFF;

			cx_hmac_sha512(arg_node + 32, 32, temp, 37, temp);

			if (cx_math_cmp(temp, (uint8_t*) app_seedutils_secp256k1_order, 32) >= 0) {
				failed = true;
			} else {
				cx_math_addm(temp, temp, arg_node, (uint8_t*) app_seedutils_secp256k1_order, 32);
				failed = cx_math_is_zero(temp, 32) != 0;
			}
			if (!failed)
				break;
			temp[0] = 1;
			os_memmove(temp + 1, temp + 32, 32);
		}
		os_memcpy(arg_node, temp, 32);
		os_memcpy(arg_node + 32, temp + 32, 32);
	}
	// Derive the node m / app_seedutils_compare_path (hardened) from the device master seed using a syscall.
	uint8_t dev_node[64];
	os_perso_derive_node_bip32(CX_CURVE_SECP256K1, (uint32_t*) &app_seedutils_compare_path, 1, dev_node, dev_node + 32);
	// Compare argument derived node to device derived node
	for (uint8_t i = 0; i < 64; i++) {
		if (arg_node[i] != dev_node[i])
			return false;
	}
	return true;
}

bool app_seedutils_valid_checksum(const char *mnemonic, uint8_t mnemonic_len) {
	// 12 word mnemonic:
	// ent[bit 0 : bit 128] = ENT
	// ent[bit 128 : bit 132] = CS
	// ent[bit 132 : 264] = 0
	// 18 word mnemonic:
	// ent[bit 0 : bit 192] = ENT
	// ent[bit 192 : bit 198] = CS
	// ent[bit 198 : 264] = 0
	// 24 word mnemonic:
	// ent[bit 0 : bit 256] = ENT
	// ent[bit 256 : bit 264] = CS
	uint8_t ent[33];
	os_memset(ent, 0, sizeof(ent));
	uint8_t word_count = 0;
	do {
		uint8_t word_len = 0;
		for (const char *word = mnemonic; *word != ' ' && *word != '\0'; word++)
			word_len++;
		app_seedutils_set_uint11(ent, word_count, app_seedutils_bip39_index(mnemonic, word_len));
		mnemonic += word_len;
		word_count++;
	} while (*mnemonic++ != '\0');
	switch (word_count) {
	case 12: {
		uint8_t hash[32];
		cx_hash_sha256(ent, 16, hash);
		return ent[16] == (hash[0] & 0xF0);
	} break;
	case 18: {
		uint8_t hash[32];
		cx_hash_sha256(ent, 24, hash);
		return ent[24] == (hash[0] & 0xFC);
	} break;
	case 24: {
		uint8_t hash[32];
		cx_hash_sha256(ent, 32, hash);
		return ent[32] == hash[0];
	} break;
	// Impossible case
	default:
		return false;
	}
}

//----------------------------------------------------------------------------//
//                                                                            //
//                       Internal Function Definitions                        //
//                                                                            //
//----------------------------------------------------------------------------//

// +========+=================+=================+=================+
// | dest_i | arr[0] = n >> x | arr[1] = n >> x | arr[2] = n >> x |
// +========+=================+=================+=================+
// |      0 |               3 |              -5 |             N/A |
// |      1 |               4 |              -4 |             N/A |
// |      2 |               5 |              -3 |             N/A |
// |      3 |               6 |              -2 |             N/A |
// |      4 |               7 |              -1 |             N/A |
// |      5 |               8 |               0 |             N/A |
// |      6 |               9 |               1 |              -7 |
// |      7 |              10 |               2 |              -6 |
// +========+=================+=================+=================+
void app_seedutils_set_uint11(uint8_t *arr, uint8_t i, uint16_t n) {
	uint16_t desti = i * 11;
	arr += desti / 8;
	desti %= 8;
	// Write to first byte
	arr[0] |= n >> (desti + 3);
	// Write to second byte
	if (desti <= 4)
		arr[1] |= (n << (5 - desti)) & 0xFF;
	if (desti >= 5)
		arr[1] |= (n >> (desti - 5)) & 0xFF;
	// Write to third byte
	if (desti >= 6)
		arr[2] |= (n << (13 - desti)) & 0xFF;
}
