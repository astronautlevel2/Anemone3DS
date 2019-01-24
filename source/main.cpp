/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2018 Contributors in CONTRIBUTORS.md
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include "Anemone3DS.h"

int __stacksize__ = 64 * 1024;
// Linear heap is used for images to display only, so it doesnt need to be a full 32 MiB if we know its maximum size beforehand
// In normal mode, with badge menu selected, each menu having more than or just enough icons, a preview saved, and badge displaying a preview while having a previous one
// (64*64*2)*12*2 + (64*64*4)*9 + (512*512*4)*2 + (1024*512*4)*2
// alternatively
// In browser mode, displaying a preview while having a previous one, each normal menu having more than or just enough icons and a preview saved
// (64*64*2)*12*2 + (64*64*4)*9 + (512*512*4)*2 + (1024*512*4) + (64*64*2)*6*4 + (512*512*4)*2
// Both are less than 7 MiB
u32 __ctru_linear_heap_size = 8*1024*1024;
bool have_sound = false;
bool running = true;
bool power_pressed = true;
bool have_luma = true;
bool running_from_hax = false;

int main(int argc, char* argv[])
{
    auto anemone = new Anemone3DS;

    while(aptMainLoop() && running)
        anemone->update();

    delete anemone;

    return 0;
}
