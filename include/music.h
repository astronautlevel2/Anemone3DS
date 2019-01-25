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

#ifndef MUSIC_H
#define MUSIC_H

#include "common.h"

#include <tremor/ivorbisfile.h>
#include <tremor/ivorbiscodec.h>

class MusicBase {
    public:
        ~MusicBase();
        void pause();
        void resume();

        bool ready = false;
        void* buf;
        u32 size;
        LightEvent stop_event, ready_or_not_event, pause_event, resume_event;

    protected:
        MusicBase(void* buf, u32 size);

    private:
        Thread bgm_thread = NULL;
};

class MusicFile : public MusicBase {
    public:
        MusicFile(std::unique_ptr<char[]>& ogg_buf, u32 ogg_size);

    private:
        std::unique_ptr<char[]> ogg_buf;
};

class MusicDownloaded : public MusicBase {
    public:
        MusicDownloaded(std::unique_ptr<u8[]>& ogg_buf, u32 ogg_size);

    private:
        std::unique_ptr<u8[]> ogg_buf;
};

#endif
