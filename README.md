![# Anemone3DS](https://github.com/astronautlevel2/Anemone3DS/blob/master/meta/banner.png)

A Theme and Splashscreen Manager for the Nintendo 3DS, written in C.

# Dependencies
 * Make, Git and PKG-Config
 * devkitARM, which can be installed following the instructions [here](https://devkitpro.org/wiki/Getting_Started).
 * jansson, libvorbisidec, libpng, and libarchive, which can be retrieved from [devkitPro pacman](https://devkitpro.org/viewtopic.php?f=13&t=8702).
 * A recent build of [makerom](https://github.com/profi200/Project_CTR) and the latest release of [bannertool](https://github.com/Steveice10/bannertool). These must be added to your PATH.

# Building
First of all, make sure devkitARM is properly installed - `$DEVKITPRO` and `$DEVKITARM` should be set to `/opt/devkitpro` and `$DEVKITPRO/devkitARM`, respectively.  
After that, open the directory you want to clone the repo into, and execute  
`git clone https://github.com/astronautlevel2/Anemone3DS` (or any other cloning method).  
To install the prerequisite libraries, begin by ensuring devkitPro pacman (and the base install group, `3ds-dev`) is installed, and then install the dkP packages `3ds-jansson`, `3ds-libvorbisidec`, `3ds-libpng`, `3ds-lz4`, `3ds-libarchive` and `3ds-curl` using `[sudo] [dkp-]pacman -S <package-name>`.  System wide packages `make`, `git` and `pkg-config` are also needed.

After adding [makerom](https://github.com/profi200/Project_CTR) and [bannertool](https://github.com/Steveice10/buildtools) to your PATH, just enter your directory and run `make`. All built binaries will be in `/out/`.

# License
This project is licensed under the GNU GPLv3. See LICENSE.md for details. Additional terms 7b and 7c apply to this project.

# Credits
The following people contributed to Anemone3DS in some way. Without these people, Anemone3DS wouldn't exist, or wouldn't be as good as it is: [CONTRIBUTORS.md](CONTRIBUTORS.md)

Most of the icons under `romfs` are from the site [icons8.com](https://icons8.com) and are licensed under the [CC-BY-NC-SA](https://creativecommons.org/licenses/by-nc-sa/3.0/)

Special thanks go to these people who, whilst not directly contributing, helped immensely:
 * [Rinnegatamante](https://github.com/Rinnegatamante), whose code served as reference on theme installation.
 * [SteveIce10](https://github.com/SteveIce10), whose QR code in FBI was essential.
 * [BernardoGiordano](https://github.com/BernardoGiordano) for making pp2d, and being super responsive to feature requests and just general help.
 * [yellows8](https://github.com/yellows8) for his home menu extdump tool, which was invaluable in debugging.
 * [MrCheeze](https://github.com/MrCheeze) and [AntiMach](https://github.com/AntiMach) whose GYTB and ABE code served as a reference on badge management
 * [Tobid7](https://github.com/tobid7) whose BCSTM-Player project served as a reference on the BCSTM format
 * The folks on #dev of Nintendo Homebrew, who helped with unicode shenanigans (especially [Stary2001](https://github.com/Stary2001), [Fenrir](https://github.com/FenrirWolf), and DanielKO).
 * The maintainers for all used libraries, including but not limited to ctrulib, libarchive, citro3d, citro2d, and quirc. An especially big thanks to the devkitPro team for maintaining a phenomenal toolchain.
 * All the people who helped keep me going and motivated me to work. This includes, but is definitely not limited to:
 
   + The members of the [Nintendo Homebrew Discord](https://discord.gg/C29hYvh)
   + The members of the __Secret Shack Service Discord__   
   + The members of the [ThemePlaza Discord](https://discord.gg/2hUQwXz)
