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

#include "text.h"

static C2D_TextBuf static_text_buf;
std::vector<std::vector<C2D_Text>> static_texts(TEXT_TYPES_AMOUNT);
std::vector<char*> keyboard_shown_text;

static const std::string& parse_line(const char* start, const char* end)
{
    static std::string out;
    out.clear();

    do {
        char character = *(start++);
        if(character == '#')
        {
            if((character = *(start++)) == 'D')
            {
                const char* to_append = nullptr;
                switch((character = *(start++)))
                {
                    case 'u':
                        to_append = "\uE079";
                        break;
                    case 'd':
                        to_append = "\uE07A";
                        break;
                    case 'l':
                        to_append = "\uE07B";
                        break;
                    case 'r':
                        to_append = "\uE07C";
                        break;
                    case 'n':
                        to_append = "\uE006";
                        break;
                    default:
                        out.push_back('#');
                        out.push_back('D');
                        out.push_back(character);
                        break;
                }
                if(to_append)
                    out.append(to_append);
            }
            else
            {
                const char* to_append = nullptr;
                switch(character)
                {
                    case 'n':
                        to_append = "\n";
                        break;
                    case 'A':
                        to_append = "\uE000";
                        break;
                    case 'B':
                        to_append = "\uE001";
                        break;
                    case 'X':
                        to_append = "\uE002";
                        break;
                    case 'Y':
                        to_append = "\uE003";
                        break;
                    case 'L':
                        to_append = "\uE004";
                        break;
                    case 'R':
                        to_append = "\uE005";
                        break;
                    default:
                        out.push_back('#');
                        out.push_back(character);
                        break;
                }
                if(to_append)
                    out.append(to_append);
            }
        }
        else if(character != '\r')
        {
            out.push_back(character);
        }
    } while(start <= end);

    return out;
}

static void load_text(TextType type, const std::string& path)
{
    auto& current_texts = static_texts[type];
    DEBUG("Loading text from %s\n", path.c_str());
    FILE* fh = fopen(path.c_str(), "r");
    bool done = false;

    do {
        char* line = nullptr;
        size_t n = 0;
        ssize_t read_length = __getline(&line, &n, fh);
        if(line && line[0] != '|')
        {
            if(line[read_length-1] != '\n')
                done = true;

            const std::string& text = parse_line(line, line + read_length - 1 - (done ? 0 : 1));
            current_texts.emplace_back();
            C2D_TextParse(&current_texts.back(), static_text_buf, text.c_str());
            C2D_TextOptimize(&current_texts.back());
        }
        free(line);
    } while(!done);

    fclose(fh);
}

static void load_text_directly(const std::string& path)
{
    DEBUG("Loading text directly from %s\n", path.c_str());
    FILE* fh = fopen(path.c_str(), "r");
    bool done = false;
    keyboard_shown_text.reserve(KEYBOARD_TEXTS_AMOUNT);

    do {
        char* line = nullptr;
        size_t n = 0;
        ssize_t read_length = __getline(&line, &n, fh);
        if(line && line[0] != '|')
        {
            if(line[read_length-1] != '\n')
                done = true;

            const std::string& text = parse_line(line, line + read_length - 1 - (done ? 0 : 1));
            keyboard_shown_text.push_back(strdup(text.c_str()));
        }
        free(line);
    } while(!done);

    fclose(fh);
}

void init_text()
{
    static_text_buf = C2D_TextBufNew(0x2000);

    fs::path lang_folder = "romfs:/lang";
    u8 language = CFG_LANGUAGE_EN;
    CFGU_GetSystemLanguage(&language);
    switch(language)
    {
        case CFG_LANGUAGE_EN:
        default:
            lang_folder /= "en";
            break;
    }
    load_text(TEXT_GENERAL, lang_folder / "text.txt");
    load_text(TEXT_INSTALL, lang_folder / "installs.txt");
    load_text(TEXT_ERROR, lang_folder / "errors.txt");
    load_text(TEXT_INSTRUCTIONS, lang_folder / "instructions.txt");
    load_text_directly(lang_folder / "keyboard.txt");

    static_texts[TEXT_GENERAL].emplace_back();
    C2D_TextParse(&static_texts[TEXT_GENERAL].back(), static_text_buf, " ");
    C2D_TextOptimize(&static_texts[TEXT_GENERAL].back());
    static_texts[TEXT_GENERAL].emplace_back();
    C2D_TextParse(&static_texts[TEXT_GENERAL].back(), static_text_buf, VERSION);
    C2D_TextOptimize(&static_texts[TEXT_GENERAL].back());

    for(auto& texts_vec : static_texts)
        texts_vec.shrink_to_fit();
}

void exit_text()
{
    for(auto& text : keyboard_shown_text)
        free(text);
    C2D_TextBufDelete(static_text_buf);
}
