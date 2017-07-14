u8 regionCode;
u32 archive1;
u32 archive2;

FS_Archive ArchiveSD;
FS_Archive ArchiveHomeExt;
FS_Archive ArchiveThemeExt;

s8 prepareThemes();
s8 themeInstall(const char* path, bool music);
s8 closeThemeArchives();
