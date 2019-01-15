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

struct SMDH {
    u8 _padding1[4 + 2 + 2];

    u16 name[0x40];
    u16 desc[0x80];
    u16 author[0x40];

    u8 _padding2[0x2000 - 0x200 + 0x30 + 0x8];
    u16 small_icon[24*24];

    u16 big_icon[48*48];
} PACKED;

class Entry {
    public:
        Entry(const fs::path& path, bool is_zip, bool directly_load = true);
        void draw() const;

        EntryIcon* load_icon() const;
        virtual PreviewImage* load_preview() const;

        void delete_entry();
        std::pair<std::unique_ptr<char[]>, u64> get_file(const std::string& file_path, u32 wanted_size = 0) const;
        bool get_file(const std::string& file_path, void* buf, u32 wanted_size = 0) const;

        template<typename T>
        u32 get_file(const std::string& file_path, T* buf, u32 wanted_size = 0) const
        {
            return this->get_file(file_path, static_cast<void*>(buf), wanted_size);
        }

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

    protected:
        SMDH* get_smdh() const;
        // takes ownership of the pointer and deletes it when it's done
        void load_meta(SMDH* icon = nullptr);
        bool is_zip;
};

#endif
