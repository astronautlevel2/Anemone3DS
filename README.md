![# Anemone3DS](https://github.com/astronautlevel2/Anemone3DS/blob/master/meta/banner.png)

A Theme and Splashscreen Manager for the Nintendo3DS, written in C

# Dependencies
 * zlib, which can be retrieved from the [3ds_portlibs](https://github.com/devkitPro/3ds_portlibs)
 * [makerom](https://github.com/profi200/Project_CTR) and [bannertool](https://github.com/Steveice10/buildtools), which can be retrieved from [SteveIce10's](https://github.com/Steveice10) buildtools repo. These must be added to your PATH.
 * [pp2d](https://github.com/BernardoGiordano/pp2d), which is included in the repo if you do a git clone --recursive.

# License
This project is licensed under the GNU GPLv3. See LICENSE.md for details. Additional terms 7b and 7c apply to this project.

# Credits
The following people contributed to Anemone3DS in some way. Without these people, Anemone3DS wouldn't exist, or wouldn't be as good as it is:
 * [Daedreth](https://github.com/daedreth), who wrote the initial implementation of theme application code and SMDH parsing.
 * [LiquidFenrir](https://github.com/LiquidFenrir), who refactored a lot of my messy GUI code and wrote the image preview from zip code, as well as the icon code.
 * [Sono](https://github.com/MarcuzD), who wrote the BCSTM playback code.
 * [Kenn (mattkenster)](https://github.com/mattkenster), for designing the GUI, a number of sprites used in the application, and drawing the banner and icon.

Special thanks go to these people who, while not directly contributing, helped immensely:
 * [Rinnegatamante](https://github.com/Rinnegatamante), whose code served as reference on theme installation.
 * [BernardoGiordano](https://github.com/BernardoGiordano) for making pp2d, and being super responsive to feature requests and just general help
 * [yellows8](https://github.com/yellows8) for his home menu extdump tool, which was invaluable in debugging.
 * the folks on #dev of Nintendo Homebrew, who helped with unicode shenanigans (especially [Stary2001](https://github.com/Stary2001), [Fenrir](https://github.com/FenrirWolf), and DanielKO).
 * the maintainers for all used libraries, including ctrulib, zlib, citro3d, pp2d and minizip.
 * all the people who helped keep me going and motivated me to work. This includes, but is definitely not limited to:
 
   + The members of the [Nintendo Homebrew Discord](https://discord.gg/C29hYvh)
   + The members of the Secret Shack Service Discord
   + The members of the [ThemePlaza Discord](https://discord.gg/2hUQwXz)
