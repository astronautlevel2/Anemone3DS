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

Entry::Entry(const fs::path& path, bool is_zip, bool directly_load) : path(path), is_zip(is_zip)
{
    if(directly_load)
        this->load_meta();
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

std::pair<std::unique_ptr<char[]>, u64> Entry::get_file(const std::string& file_path, u32 wanted_size) const
{
    if(this->is_zip)
    {
        return zip_file_to_buf(file_path.c_str(), this->path, wanted_size);
    }
    else
    {
        std::string full_path = this->path / file_path;
        return file_to_buf(fsMakePath(PATH_ASCII, full_path.c_str()), SD_CARD, wanted_size);
    }
}

bool Entry::get_file(const std::string& file_path, void* buf, u32 wanted_size) const
{
    if(this->is_zip)
    {
        return zip_file_to_buf(file_path.c_str(), this->path, buf, wanted_size);
    }
    else
    {
        std::string full_path = this->path / file_path;
        return file_to_buf(fsMakePath(PATH_ASCII, full_path.c_str()), SD_CARD, buf, wanted_size);
    }
}

EntryIcon* Entry::load_icon() const
{
    if(this->color)
        return nullptr;

    EntryIcon* out = nullptr;

    if(this->is_zip && this->path.extension() == ".png")
    {
        out = new(std::nothrow) BadgeIcon(this->path);
    }
    else
    {
        SMDH* icon = this->get_smdh();
        if(icon != nullptr)
        {
            out = new(std::nothrow) EntryIcon(icon->big_icon);
            delete icon;
        }
    }

    return out;
}

std::pair<std::unique_ptr<u8[]>, u32> Entry::download_remote_entry(char** filename)
{

}

PreviewImage* Entry::load_preview() const
{
    if(this->is_zip && this->path.extension() == ".png")
    {
        return new(std::nothrow) BadgePreviewImage(this->path);
    }
    else
    {
        const auto& [png_buf, png_size] = this->get_file("preview.png");
        if(png_size)
        {
            PreviewImage* preview = new(std::nothrow) PreviewImage(png_buf.get(), png_size);
            return preview;
        }
        else
        {
            draw_error(ERROR_LEVEL_ERROR, ERROR_TYPE_NO_PREVIEW);
            return nullptr;
        }
    }
}

void Entry::delete_entry()
{
    FS_Path path = fsMakePath(PATH_ASCII, this->path.c_str());
    if(this->is_zip)
        delete_file(path, SD_CARD);
    else
        delete_folder(path, SD_CARD);
}

SMDH* Entry::get_smdh() const
{
    SMDH* out = new(std::nothrow) SMDH;
    if(this->get_file("info.smdh", out, sizeof(SMDH)))
        return out;

    delete out;
    return nullptr;
}

void Entry::load_meta(SMDH* icon)
{
    if(this->path.extension() != ".png")  // Theme or Splash, but not Bagde
    {
        if(icon == nullptr)
            icon = this->get_smdh();
        if(icon != nullptr)
        {
            char utf_title[0x40] = {0};
            utf16_to_utf8(reinterpret_cast<u8*>(utf_title), icon->name, 0x40);
            this->title = std::string(utf_title, 0x40);

            char utf_description[0x80] = {0};
            utf16_to_utf8(reinterpret_cast<u8*>(utf_description), icon->desc, 0x80);
            this->description = std::string(utf_description, 0x80);

            char utf_author[0x40] = {0};
            utf16_to_utf8(reinterpret_cast<u8*>(utf_author), icon->author, 0x40);
            this->author = std::string(utf_author, 0x40);
            delete icon;
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
        this->title = this->path.stem();
}
