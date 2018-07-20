 ----------------------------------------------------------------------------
 SMS Plus
 ----------------------------------------------------------------------------

 Version 0.9.4a
 by Charles Mac Donald

 What's New
 ----------

 [Version 0.9.4a]

 - A few cleanups so SMS Plus compiles with no errors or warnings.
 - Use of MMX code is now automatic but can still be disabled manually.
 - Replaced FM sound emulation with Mitsutaka Okazaki's EMU2413 library.
 - Fixed parts of the VDP emulation.
 - Removed support for PSX port.

 [Version 0.9.3]

 - Stereo Game Gear sound.
 - Save states, 10 per game.
 - Sound logging, output is .GYM compatible.
 - Optimized memory access for improved speed.
 - Screen snapshots in PCX format.
 - Stereo swap option for left and right speakers.
 - Added FPS meter.
 - Added some option toggle keys for in-game configuration.
 - Definable video driver.
 - Tweaked display modes for full-screen gameplay.
 - More of the usual internal changes and bug fixes.

 [Version 0.9.2]

 - Digital YM2413 FM sound emulation. Drums sound real good, too.
 - System territory switch, default is overseas (Europe / USA)
 - Made some internal changes and clean-ups to the source code.
 - Revised document on porting to be actually useful.
 - Adjusted version number to stay in sync with the Macintosh port.
 - Support for ZIPped game images and games with 512-byte headers.
 - Screen expansion now works for 16-bit displays.
 - Fixed some centering problems with screen expansion on GG games.
 - Sound output using SEAL should be a bit clearer.
 - Configuration file support.

 [Version 0.9]

 Everything has been rewritten from scratch. The emulation is much more
 accurate, as a result compatability has improved greatly.

 The DOS port has a lot of nice features added, including 16-bit color,
 display blurring, digital sound emulation, and scanlines, which were the
 most requested items.

 Also new is a Sony Playstation port, though it's highly experimental
 and has some serious performance issues.

 SMS Plus is now free software; the source code is available for anyone
 to use as they see fit, and is distributed under the terms of the GNU
 General Public License.

 Usage
 -----

 (DOS)

 You'll need at least a Pentium 133, a VGA compatible display card,
 and optionally a joystick and sound card.

 Controls are as follows:

 Arrow Keys -   Directional pad
 Control    -   Button II
 Alt        -   Button I
 Enter      -   Start (GG) / PAUSE (SMS)
 Tab        -   Soft reset (SMS) / Hard reset (GG)
 Delete     -   Hard reset
 Esc/End    -   Exit program
 0-9        -   Select save state slot
 Backspace  -   Start/stop music logging.

 F1-F4      -   Set frameskip level (F1 = no skip ... F4 = skip 3 frames)
 F5         -   Save state file from current slot
 F6         -   Cycle through state slots (0-9)
 F7         -   Save state file to current slot
 F8         -   Make PCX screen snapshot
 F9         -   Toggle VSync
 F10        -   Toggle speed throttling
 F11        -   Toggle FPS meter

 You can only support a second player if you are using a joystick driver
 that supports more than one joystick. (e.g. Sidewinder, dual pads, etc.)

 Type 'sp -help' on the command line for a list of useful options.

    -res <x> <y>    set the display resolution.
    -vdriver <n>    specify video driver.
    -depth <n>      specify color depth. (8, 16)
    -blur           blur display. (16-bit color only)
    -scanlines      use scanlines effect.
    -tweak          force tweaked 256x192 or 160x144 8-bit display.
    -scale          scale display to full resolution. (slow)
    -expand         force 512x384 or 400x300 zoomed display.
    -nommx          disable use of MMX instructions.
    -novga          disable use of VGA vertical scaling with '-expand'.
    -vsync          wait for vertical sync before blitting.
    -throttle       limit updates to 60 frames per second.
    -fps            show FPS meter.
    -sound          enable sound. (force speed throttling)
    -sndrate <n>    specify sound rate. (8000, 11025, 22050, 44100)
    -sndcard <n>    specify sound card. (0-7)
    -swap           swap left and right stereo output.
    -joy <s>        specify joystick type.
    -jp             use Japanese console type.
    -fm             required to enable YM2413 sound.

 Here is a list of all the video drivers you can pass as a parameter
 to the '-vdriver' option:

    auto, safe, vga, modex, vesa2l, vesa3, vbeaf

 Here is a list of all the joystick drivers you can pass as a parameter
 to the '-joy' option:

    auto, none, standard, 2pads, 4button, 6button, 8button, fspro, wingex,
    sidewinder, gamepadpro, grip, grip4, sneslpt1, sneslpt2, sneslpt3,
    psxlpt1, psxlpt2, psxlpt3, n64lpt1, n64lpt2, n64lpt3, db9lpt1, db9lpt2,
    db9lpt3, tglpt1, tglpt2, tglpt3, wingwar, segaisa, segapci, segapci2

 If you use the expand option without scanlines, and the display looks
 squashed vertically, then also use the '-novga' switch.

 You can disable MMX use with the '-nommx' switch.

 You can put any commandline option into a plain text file which should
 be called "sp.cfg". Put one option per line, please. Command line options
 will override anything in the configuration file.

 Battery back-up RAM and save states are saved to the same directory as the
 game you loaded. This means running games off write-only media will not
 work. These files are named after the name of the image itself, not the
 name of the zipfile archive in the case of zip loading.

 Currently the zip loading code can manage a zipfile where the game
 image is the first thing in it. If you try to open a huge archive of
 games, only the first will be played.

 Sound Logging
 -------------

 You can log sound output to a file with the extension SSL.
 Currently there is a limit of 100 music logs generated per game.

 The format of this file is not compatible with GYM files generated by
 emulators such as genecyst and Megasis.

 Format of an SSL file: (taken from genecyst's readme)

 while (!EOF)
   switch (readbyte()) // readbyte() reads the next byte from the file
   {
    case 0: 1/60th of a second has elapsed; 
    case 1: ymport=0; ymreg=readbyte(); ymdata=readbyte();
    case 2: ymport=1; ymreg=readbyte(); ymdata=readbyte();
    case 3: write readbyte() to psg port
    case 4: psg_stereo=readbyte();
    case 5: ym2413_reg_latch=readbyte();
    case 6: ym2413_data_port=readbyte();
   }

 The format of the PSG stereo byte in case 4 is:

 bit 0 : Left speaker noise enable
 bit 1 : Left speaker tone #3 enable
 bit 2 : Left speaker tone #2 enable
 bit 3 : Left speaker tone #1 enable
 bit 4 : Right speaker noise enable
 bit 5 : Right speaker tone #3 enable
 bit 6 : Right speaker tone #2 enable
 bit 7 : Right speaker tone #1 enable

 Thanks
 ------

 All Allegro contributors, Bero, Carlos Hasan, Dave, Eric Quinn,
 Frank Hughes, Flavio Morsoletto, Gilles Volant, Hiromitsu Shioya, Jon,
 Jean-loup Gailly, James McKay, Kreed, Mark Adler, Micheal Cunanan,
 Marcel de Kogel, Marat Fayzullin, Mitsutaka Okazaki, Nyef, Nick Jacobson,
 Omar Cornut, Paul Leaman, Ricardo Bittencourt, Richard Mitton,
 Richard Talbot-Watkins, Sean Young, Tatsuyuki Satoh, the MAME team, and
 the S8-DEV forum members.

 Richard Bannister for the Macintosh port. (www.bannister.org)
 Richard Teather for the Win32 port. (smsplus.vintagegaming.com)
 Caz Jones for the BeOS port. (http://www.infernal.currantbun.com)
 Cyx for the SDL port. (http://membres.lycos.fr/cyxdown/smssdl/)
 Ulrich Hecht for the Linux port. (http://www.emulinks.de/emus/)
 ss_teven for the Sega Dreamcast port. (no homepage URL)
 Dagolar for the Sega Saturn port. (http://phemusat.tripod.com/)
 Aj0 for the GP32 port. (http://ajo.thinknerd.com/gp32/sms32/files/)
 ? for the X-Box port. (http://xport.xb-power.com/smsplus.html)
 ? for the Sony Playstation 2 port. (http://nik.napalm-x.com/)

 The artists who worked on Red Zone and Sub Terrania for the cool font.

 I'd also like to thank everybody who contributed information, bug reports,
 and gave their comments and ideas.

 Contact
 -------

 Charles Mac Donald
 E-mail: cgfm2@hotmail.com
 WWW: http://cgfm2.emuviews.com

 Legal
 -----

 The source code is distributed under the terms of the GNU General Public
 License.

 The excellent EMU2413 emulator is written by Mitsutaka Okazaki.
 (http://www.angel.ne.jp/~okazaki/ym2413/)

 The Z80 CPU emulator and SEAL interface code are taken from the MAME project,
 and terms of their use are covered under the MAME license.
 (http://www.mame.net)

 The blur code is based on the 'CTV' sources from Dave's DGen emulator.
 (http://www.dtmnt.com)

