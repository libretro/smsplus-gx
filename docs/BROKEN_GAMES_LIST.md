Known issues
=============

-  93c46 EEPROM Support

Used by a few Game Game games.

Majors Pro Baseball
Nomo's World Series Baseball (J)
Pro Yakyuu GG League (J)
World Series Baseball [A]
World Series Baseball [B]
World Series Baseball 95

Implementations :
https://github.com/OpenEmu/CrabEmu-Core/blob/master/consoles/sms/mapper-93c46.c

World Series Baseball '95 and World Series Baseball will hang up without at least a stub in place.

-  Fix PAL specific code

Active Range should be 294 for PAL but it had to be set to 267 to make Fantastic Dizzy work properly.
Unfortunately this is still not quite correct, we need a proper fix for it.

- LCD Persistence issues

Some games take advantage of the Game Gear's poor LCD screen.
If this behaviour is not replicated then it results in some heavy flickering.

Here's a list of affected games :

-  The Adventures of Batman & Robin (Disappearing HUD)
-  Super Monaco GP 2 (Titlescreen)
-  Space Harrier
-  Ninja Gaiden (affects 3rd & 2nd act)
-  Halley Wars
-  MLBPA Baseball
-  Power Drive (unreleased prototype)

MAME has an implementation of LCD persistence here (first appeared in MESS 0.135) :

https://github.com/mamedev/mame/blob/08cd6110268f9363e6c2cb9f0b6d92c00dadfbd6/src/mame/machine/sms.cpp#L1557

-  Graphics Board

http://www.smspower.org/forums/15032-SegaGraphicBoardV20ReverseEngineering
No support for it given how recent it is. MEKA supports this so i should take a look at it.

-  Space Gun

http://www.smspower.org/Development/SpaceGun-SMS

I need to add back Lightgun emulation.
The code is left turned off for now because most games also support the controllers but this game requires the light gun.

There's also an issue with memory mapping. I believe we don't suffer from that bug ?

-  Sports Pad

Not currently implemented.
In addition to that, a bug has been discovered where the game would sometimes read the input from player 1 for player 2.
Do we really want to implement this bug though ???

https://github.com/mamedev/mame/commit/33c5196a429d0afa801caac3ad24a76bf0ac273d#diff-cafb9b1c2517c95f421c776c3ec74e56

-  Support for 4PAK Mapper

Mostly similar to Codemasters's writemap function except for 8000-0xBFFF range.

Write xx to 3FFE: map bank (xx)&maxbankmask to memory slot in 0000-3FFF region
Write yy to 7FFF: map bank (yy)&maxbankmask to memory slot in 4000-7FFF region
Write zz to BFFF: map bank ((xx&0x30)+(zz))&maxbankmask to memory slot in 8000-BFFF region

Implementations :
https://github.com/ocornut/meka/blob/e0f6d9eb89217c4a083fe09b71900c5fa9321abf/meka/srcs/mappers.c#L402
https://github.com/OpenEmu/CrabEmu-Core/blob/a25c7277cd14f9e32647e1b9e1a45621fb051f13/consoles/sms/mapper-4PAA.c


Problematic games (To be checked & tested)
=================================

-   Alibaba and 40 Thieves / Block Hole 

http://www.smspower.org/Development/AlibabaAnd40Thieves-SMS
Looks like it will need more research.

-  California Games II

http://www.smspower.org/Development/CaliforniaGamesII-SMS
Sonic's Edusoft had a similar issue with it expecting VDP Register 1 to be set to 0xE0, which is what the BIOS sets it to.

-  Dallyeora Pigu Wang

http://www.smspower.org/Development/DallyeoraPiguWang-SMS

Uses R register and Korean mapper. (Filesize is also not a power of 2)
Sounds like a problem.

-  James Pond 3 - Operation Starfi5h (Game Gear)

http://www.smspower.org/Development/JamesPond3-GG

- Madou Monogatari 2 (Game Gear)

Marat Fayzullin mentionned this in the changelog of his MasterGear emulator.
"Fixed Madou Monogatari 2 (GG) by setting VDP[6]=0xFF on reset."

This is a very obscure hardware behaviour, i haven't seen this documented anywhere.
SMS Plus GX works without the fix in place. Saving/loading also works.
Maybe it's an unrelated issue on his side ?

-  Rainbow Islands

http://www.smspower.org/Development/RainbowIslands-SMS
Brazil release fixed this, perhaps we could patch the game on the fly ?
But that would be outside the scope of an emulator.

-  PGA Tour Golf

http://www.smspower.org/Development/PGATourGolf-SMS

-  Zool : Ninja of the Nth Dimension

http://www.smspower.org/Development/Zool-SMS
