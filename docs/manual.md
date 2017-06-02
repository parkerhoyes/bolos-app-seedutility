# BOLOS Seed Utility App Manual

The BOLOS Seed Utility app for the [Ledger Nano
S](https://github.com/LedgerHQ/ledger-nano-s) is intended to provide various
utilities for working with [BIP
39](https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki) mnemonic
seeds. Currently, this app has one feature: [verifying the
backup](#verify-backup) of your BIP 39 mnemonic seed (the copy you wrote down on
paper). This manual outlines the various sections and features of the app, how
to access them, and how they work.

All screenshots in this manual are actual pictures of the app running on the
Nano S, but they've been heavily Photoshopped to make them easy to see on the
computer.

# Main Menu

![Main Menu](pictures/main_menu.png)

The main menu of the app has a vertically-scrolling menu that lets you choose
from the following three options:

- **Verify Backup** - Takes you to the [verify backup](#verify-backup) screen
- **About** - Displays the app version number and author information
- **Quit app** - Spawns unicorns

# Verify Backup

The verify backup feature allows you to enter a 12, 18, or 24 word seed that you
have backed up somewhere, and the app will compare that seed to the mnemonic on
the device and tell you if they match. **This feature does not allow you to
extract your mnemonic from the device**; doing so is impossible from a
user-level application as that would be a major security issue.

When you enter this screen, you will be faced with a vertically-scrolling menu
that asks you the length of your BIP 39 mnemonic seed (12, 18, or 24 words).
This app does not support BIP 39 mnemonic seeds of other lengths, as no other
lengths are supported by the Ledger Nano S at the time of writing.

Once you select your seed's length, you will see another vertically-scrolling
menu with elements that look something like this:

![Enter Seed 1](pictures/enter_seed_1.png)

By pressing both the left and right buttons on the Nano S at the same time, you
can select a word to enter (or replace). Upon doing so, you will see a screen
that looks similar to this:

![Enter Word](pictures/enter_word.png)

This app uses a [binary keyboard
](https://github.com/parkerhoyes/bolos-user-interface#binary-keyboard-module)
(which you may be familiar with from [Ledger's Password Manager
app](https://github.com/LedgerHQ/blue-app-password-manager)) to type in words.
The binary keyboard words like so:

There are two groups of letters on the screen: one on the left, and one on the
right. The textbox and your cursor is displayed at the bottom of the screen.
Locate the next letter in the word you would like to type (in the image above,
I'm trying to type "torch", so the next letter I would want to type is R). Press
the button on the Nano S corresponding to the side that your letter is on (in my
case, I would press the right button). After pressing a button, all of the
letters on the opposite side of the screen will disappear and the entire screen
will reorganize. Repeat this process to type in a letter.

After typing at most four letters, the app will know what word you want to type
in and will display a confirmation screen like the following:

![Word Prediction](pictures/word_prediction.png)

Press the right button to confirm that this is the word you were trying to type
in, or the left button to try again.

After you have typed in the word and confirmed it, the corresponding menu entry
should look something like this (for a different word than in the example
above):

![Enter Seed 2](pictures/enter_seed_2.png)

If you typed in the wrong word, you may select that word again to type in a new
word.

Once you've finished entering all of the words in your mnemonic, scroll through
the entire list to double-check that you entered every word correctly. Then,
scroll to the bottom of the screen and select "Done".

The app will take some time to think.

![Thinking](pictures/thinking.png)

Then, if the mnemonic you entered was the same as the master seed of the device,
you should see a message like this:

![Same Seed](pictures/same_seed.png)

Press both buttons to return to the main menu.
