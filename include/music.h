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
#include "fs.h"
#include "unicode.h"

#include <tremor/ivorbisfile.h>
#include <tremor/ivorbiscodec.h>

#define BUF_TO_READ 40960 // How much data should be buffered at a time

typedef struct {
    OggVorbis_File vf;
    ndspWaveBuf wave_buf[2];
    float mix[12];
    u8 buf_pos;
    long data_read;
    char *filebuf;
    u32 filesize;
    
    volatile bool stop;
    Handle finished;
} audio_s;

void play_audio(audio_s *);

#endif