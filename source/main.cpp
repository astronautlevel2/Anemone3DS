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
bool have_sound = false;
bool running = true;
bool power_pressed = true;
bool have_luma = true;

/*
Image::Image(u16 w, u16 h, GPU_TEXCOLOR format) : w(w), h(h)
{
    this->image = new C2D_Image;

    this->image->tex = new C3D_Tex;
    u16 w_nextPow2 = nextPow2(w);
    u16 h_nextPow2 = nextPow2(h);
    C3D_TexInit(this->image->tex, w_nextPow2, h_nextPow2, format);

    Tex3DS_SubTexture* subtex = new Tex3DS_SubTexture;
    subtex->width = w;
    subtex->height = h;
    subtex->left = 0.0f;
    subtex->top = 1.0f;
    subtex->right = static_cast<float>(w) / w_nextPow2;
    subtex->bottom = 1.0f - (static_cast<float>(h) / h_nextPow2);
    this->image->subtex = subtex;
}

Image::~Image()
{
    C3D_TexDelete(this->image->tex);
    delete this->image->tex;
    delete this->image->subtex;
    delete this->image;
}
*/

int main(int argc, char* argv[])
{
    auto anemone = new Anemone3DS;

    while(aptMainLoop() && running)
        anemone->update();

    delete anemone;

    return 0;
}
