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

#ifndef ENTRY_H
#define ENTRY_H

#include "common.h"
#include "preview.h"
#include "icons.h"

class Entry {
    public:
        Entry(const fs::path& path, bool is_zip);
        void draw() const;

        EntryIcon* load_icon() const;
        PreviewImage* load_preview() const;

        void delete_entry();
        u32 get_file(const std::string& file_path, char** buf) const;

        std::string title, description, author;
        fs::path path;
        u32 color = 0;

        enum EntryState {
            STATE_NONE = 0,

            STATE_SHUFFLE = BIT(0),
            STATE_SHUFFLE_NO_BGM = BIT(1),
            STATE_MULTI = BIT(2),
        };
        u32 state = STATE_NONE;  // marked for shuffle, multi-install, etc...

    private:
        bool is_zip;
};

class RemoteEntry : public Entry {
    public:
        RemoteEntry(int entry_id);
};

#endif
