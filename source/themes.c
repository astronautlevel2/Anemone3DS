#include <3ds.h>
#include <stdlib.h>

#include "themes.h"

Result single_install(theme* theme)
{
    char *body;
    char *music;
    char *savedata_buf;
    char *thememanage_buf;
    u32 body_size;
    u32 music_size;
    u32 savedata_size;

    savedata_size = file_to_buf(fsMakePath(PATH_ASCII, "/SaveData.dat"), ArchiveHomeExt, savedata_buf)
    savedata_buf[0x141b] = 0;
    memset(&savedata_buf[0x13b8], 0, 8);
    savedata_buf[0x13bd] = 3;
    savedata_buf[0x13b8] = 0xff;
    buf_to_file(savedata_size, "/SaveData.dat", ArchiveHomeExt, savedata_buf);
    free(savedata_buf);

    // Open body cache file. Test if theme is zipped
    if (theme->is_zip)
    {
        body_size = zip_file_to_buf("body_lz.bin", theme->path, body);
    } else {
        u16 path[0x106];
        memcpy(path, theme->path, 0x106);
        struacat(path, "/body_lz.bin");
        body_size = file_to_buf(path, body);
    }

    if (body_size == 0)
    {
        free(body);
        return MAKERESULT(RL_PERMANENT, RL_CANCELED, RL_APPLICATION, RD_NOT_FOUND);
    }

    if (check_file_exists("/BodyCache.bin", ArchiveThemeExt)) FSUSER_DeleteFile(ArchiveThemeExt, fsMakePath("/BodyCache.bin"));

    u32 size = buf_to_file(body_size, "/BodyCache.bin", ArchiveThemeExt, body); // Write body data to file
    free(body);

    if (size == 0) return MAKERESULT(RL_PERMANENT, RL_CANCELED, RL_APPLICATION, RD_NOT_FOUND);

    if (theme->is_zip) // Same as above but this time with bgm
    {
        music_size = zip_file_to_buf("bgm.bcstm", theme->path, music);
    } else {
        u16 path[0x106];
        memcpy(path, theme->path, 0x106);
        struacat(path, "/bgm.bcstm");
        music_size = file_to_buf(path, music);
    }

    if (music_size == 0)
    {
        free(music);
        music = calloc(1, 3371008);
    } else if (size > 3371008) {
        free(music);
        return MAKERESULT(RL_PERMANENT, RL_CANCELED, RL_APPLICATION, RD_TOO_LARGE);
    }

    if (check_file_exists("/BgmCache.bin", ArchiveThemeExt)) FSUSER_DeleteFile(ArchiveThemeExt, fsMakePath("/BgmCache.bin"));

    size = buf_to_file(music_size, "/BgmCache.bin", ArchiveThemeExt, music);
    free(music);

    if (size == 0) return MAKERESULT(RL_PERMANENT, RL_CANCELED, RL_APPLICATION, RD_NOT_FOUND);

    file_to_buf(fsMakePath(PATH_ASCII, "/ThemeManage.bin"), ArchiveThemeExt, thememanage_buf);
    thememanage_buf[0x00] = 1;
    thememanage_buf[0x01] = 0;
    thememanage_buf[0x02] = 0;
    thememanage_buf[0x03] = 0;
    thememanage_buf[0x04] = 0;
    thememanage_buf[0x05] = 0;
    thememanage_buf[0x06] = 0;
    thememanage_buf[0x07] = 0;

    u32 *body_size_location = (u32*)(&thememanage_buf[0x8]);
    u32 *music_size_location = (u32*)(&thememanage_buf[0xC]);
    *body_size_location = body_size;
    *music_size_location = music_size;

    thememanage_buf[0x10] = 0xFF;
    thememanage_buf[0x14] = 0x01;
    thememanage_buf[0x18] = 0xFF;
    thememanage_buf[0x1D] = 0x02;

    memset(&thememanage_buf[0x338], 0, 4);
    memset(&thememanage_buf[0x340], 0, 4);
    memset(&thememanage_buf[0x360], 0, 4);
    memset(&thememanage_buf[0x368], 0, 4);
    buf_to_file(0x800, "/ThemeManage.bin", ArchiveThemeExt, thememanage_buf);
    free(thememanage_buf);

    return 0;
}