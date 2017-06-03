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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APP_SEEDUTILS_WORD_LEN_MIN 3
#define APP_SEEDUTILS_WORD_LEN_MAX 8
#define APP_SEEDUTILS_WORD_COUNT 2048

/*
 * Find all possible letters that may follow the beginning of the specified word in the BIP 39 English wordlist. All
 * letters provided to this function and returned by this function must be lower case ASCII letters.
 *
 * Args:
 *     word: the start of a word
 *     word_len: the length of word
 *     letters_dest: the destination in which to store all letters that may be used to continue the word (no
 *                   null-terminator); this must be big enough to store 26 letters
 *     complete: if not NULL, this will be set to true if word is a complete BIP 39 word, or false otherwise
 *     prediction: if word is the prefix of exactly one BIP 39 word (or is an entire BIP 39 word), then this will be set
 *                 to the index of that word, otherwise this is set to -1; if this is NULL, it is not accessed
 * Returns:
 *     the number of letters stored in letters_dest; in [0, 26]
 */
uint8_t app_seedutils_bip39_next_letters(const char *word, uint8_t word_len, char *letters_dest, bool *complete,
		int16_t *prediction);

/*
 * Get the word at the specified index in the BIP 39 English wordlist.
 *
 * Args:
 *     index: the word index; must be in [0, 2047]
 *     word_len_dest: if not NULL, the length of the word is stored here
 * Returns:
 *     a pointer to the word at the specified index (the word may not have a null-terminator)
 */
const char* app_seedutils_bip39_word(uint16_t index, uint8_t *word_len_dest);

/*
 * Get the index of the specified word in the BIP 39 English wordlist.
 *
 * Args:
 *     word: the word for which to search the wordlist (null-terminator is not required)
 *     word_len: the number of characters in word
 * Returns:
 *     the index of word in the wordlist, or 2048 (APP_SEEDUTILS_WORD_COUNT) if the word was not found
 */
uint16_t app_seedutils_bip39_index(const char *word, uint8_t word_len);

/*
 * Determine if the provided BIP 39 mnemonic seed is the same as the master seed loaded on the device.
 *
 * Args:
 *     mnemonic: the BIP 39 mnemonic for the seed; this may be modified by this function, and should be considered
 *               garbage after this function returns
 *     mnemonic_len: the number of chars in mnemonic
 * Returns:
 *     true if the seed derived from mnemonic is equal to the master seed, false otherwise
 */
bool app_seedutils_compare(char *mnemonic, uint8_t mnemonic_len);

/*
 * Determine if the provided BIP 39 mnemonic seed has a valid checksum. The seed must be a sequence of 12, 18, or 24
 * valid words in the BIP 39 English wordlist, space-delimited.
 *
 * Args:
 *     mnemonic: the BIP 39 mnemonic
 *     mnemonic_len: the number of chars in mnemonic
 */
bool app_seedutils_valid_checksum(const char *mnemonic, uint8_t mnemonic_len);
