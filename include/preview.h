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

#ifndef PREVIEW_H
#define PREVIEW_H

#include "common.h"
#include "music.h"

struct PreviewImage {
    bool ready = false;
    void pause();
    void resume();

    PreviewImage(void* png_buf, u32 png_size, std::unique_ptr<char[]>& ogg_buf, u32 ogg_size);
    PreviewImage(void* png_buf, u32 png_size, std::unique_ptr<u8[]>& ogg_buf, u32 ogg_size);
    ~PreviewImage();
    virtual void draw() const;

protected:
    PreviewImage();
    C2D_Image* image;

private:
    PreviewImage(void* png_buf, u32 png_size);
    std::unique_ptr<MusicBase> bgm;
};

struct BadgePreviewImage : PreviewImage{
    BadgePreviewImage(const fs::path& path);
    void draw() const;
};

#endif
