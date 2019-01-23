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

#include "remote_entry.h"
#include "network.h"
#include "draw.h"

RemoteEntry::RemoteEntry(int entry_id) : Entry(fs::path("/3ds") / APP_TITLE / "cache" / std::to_string(entry_id), false, false), entry_id(entry_id)
{
    fs::create_directories(this->path);
    SMDH* icon = this->get_smdh();
    if(icon == nullptr)
    {
        icon = new SMDH;
        if(download_data(get_download_url(THEMEPLAZA_SMDH_FORMAT, this->entry_id), INSTALLS_AMOUNT, icon, sizeof(SMDH)))
        {
            std::string full_path = this->path / "info.smdh";
            FILE* fh = fopen(full_path.c_str(), "wb");
            fwrite(icon, sizeof(SMDH), 1, fh);
            fclose(fh);
        }
    }
    
    this->load_meta(icon);
}

std::pair<std::unique_ptr<u8[]>, u32> RemoteEntry::download_remote_entry(char** filename)
{
    draw_install(INSTALL_DOWNLOAD);
    return download_data(get_download_url(THEMEPLAZA_DOWNLOAD_FORMAT, this->entry_id), INSTALL_DOWNLOAD, filename);
}

PreviewImage* RemoteEntry::load_preview() const
{
    const auto& [file_buf, file_size] = this->get_file("preview.png");
    if(file_size)
    {
        return new(std::nothrow) PreviewImage(file_buf.get(), file_size);
    }
    else
    {
        draw_install(INSTALL_LOADING_REMOTE_PREVIEW);
        const auto& [dl_buf, dl_size] = download_data(get_download_url(THEMEPLAZA_PREVIEW_FORMAT, this->entry_id), INSTALL_LOADING_REMOTE_PREVIEW);
        std::string full_path = this->path / "preview.png";
        FILE* fh = fopen(full_path.c_str(), "wb");
        fwrite(dl_buf.get(), 1, dl_size, fh);
        fclose(fh);
        return new(std::nothrow) PreviewImage(dl_buf.get(), dl_size);
    }
}
