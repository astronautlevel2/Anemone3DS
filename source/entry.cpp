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

#include "entry.h"
#include "file.h"
#include "draw.h"

struct SMDH {
    u8 _padding1[4 + 2 + 2];

    u16 name[0x40];
    u16 desc[0x80];
    u16 author[0x40];

    u8 _padding2[0x2000 - 0x200 + 0x30 + 0x8];
    u16 small_icon[24*24];

    u16 big_icon[48*48];
} PACKED;

Entry::Entry(const fs::path& path, bool is_zip) : path(path), is_zip(is_zip)
{
    char* buf = nullptr;
    if(is_zip && path.extension() == ".png")  // Badge
    {
        this->title = path.stem();
    }
    else  // Theme or Splash
    {
        u32 size = this->get_file("info.smdh", &buf);
        if(buf != nullptr)
        {
            if(size == 0x36c0)
            {
                SMDH* smdh_buf = reinterpret_cast<SMDH*>(buf);

                char utf_title[0x40] = {0};
                utf16_to_utf8(reinterpret_cast<u8*>(utf_title), smdh_buf->name, 0x40);
                this->title = std::string(utf_title, 0x40);

                char utf_description[0x80] = {0};
                utf16_to_utf8(reinterpret_cast<u8*>(utf_description), smdh_buf->desc, 0x80);
                this->description = std::string(utf_description, 0x80);

                char utf_author[0x40] = {0};
                utf16_to_utf8(reinterpret_cast<u8*>(utf_author), smdh_buf->author, 0x40);
                this->author = std::string(utf_author, 0x40);
            }
            else
            {
                this->color = C2D_Color32(rand() % 256, rand() % 256, rand() % 256, 255);
            }
            delete[] buf;
        }
        else
        {
            this->color = C2D_Color32(rand() % 256, rand() % 256, rand() % 256, 255);
        }
    }

    if(this->author.empty())
        this->author = "Unknown author";
    if(this->description.empty())
        this->description = "No description";
    if(this->title.empty())
        this->title = "No title";
}

void Entry::draw() const
{
    static constexpr float x = 20.0f;
    float y = 35.0f;
    float height, width;
    get_text_dimensions(TEXT_GENERAL, TEXT_ENTRY_BY, &width, &height, 0.5f, 0.5f);
    draw_text(TEXT_GENERAL, TEXT_ENTRY_BY, COLOR_WHITE, x, y, 0.2f, 0.5f, 0.5f);
    draw_text(this->author, COLOR_WHITE, x + width, y, 0.2f, 0.5f, 0.5f);
    y += height + 2.0f;
    height = draw_text_wrap(this->title, COLOR_WHITE, 400.0f - x*2.0f, x, y, 0.2f, 0.7f, 0.7f);
    y += height + 8.0f;
    C2D_DrawRectSolid(12.0f, y, 0.2f, 400.0f - 12.0f*2.0f, 1.0f, COLOR_CURSOR);
    y += 1.0f + 8.0f;
    draw_text_wrap(this->description, COLOR_WHITE, 400.0f - x*2.0f, x, y, 0.2f, 0.5f, 0.5f);
}

u32 Entry::get_file(const std::string& file_path, char** buf) const
{
    if(this->is_zip)
    {
        return zip_file_to_buf(file_path.c_str(), this->path, buf);
    }
    else
    {
        std::string full_path = this->path / file_path;
        return file_to_buf(fsMakePath(PATH_ASCII, full_path.c_str()), SD_CARD, buf);
    }
}

EntryIcon* Entry::load_icon() const
{
    if(this->color)
        return nullptr;

    EntryIcon* out = nullptr;

    if(this->is_zip && this->path.extension() == ".png")
    {
        out = new BadgeIcon(this->path);
    }
    else
    {
        char* buf = nullptr;
        u32 size = this->get_file("info.smdh", &buf);
        if(buf != nullptr)
        {
            if(size == 0x36c0)
            {
                SMDH* smdh_buf = reinterpret_cast<SMDH*>(buf);
                out = new EntryIcon(smdh_buf->big_icon);
            }
            delete[] buf;
        }
    }

    return out;
}

PreviewImage* Entry::load_preview() const
{
    if(this->is_zip && this->path.extension() == ".png")
    {
        return new BadgePreviewImage(this->path);
    }
    else
    {
        char* png_buf = nullptr;
        u32 png_size = this->get_file("preview.png", &png_buf);
        PreviewImage* preview = new PreviewImage(png_buf, png_size);

        if(png_buf != nullptr)
            delete[] png_buf;

        return preview;
    }
}

void Entry::delete_entry()
{

}

RemoteEntry::RemoteEntry(int entry_id) : Entry("/3ds/"  APP_TITLE  "/cache/" + std::to_string(entry_id), false)
{

}
