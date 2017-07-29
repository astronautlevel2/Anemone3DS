#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "theme.h"
#include "splashes.h"
#include "unicode.h"
#include "ui.h"
// Taken from: https://stackoverflow.com/questions/22582989/word-wrap-program-c
inline int wordlen(const char * str){
   int tempindex=0;
   while(str[tempindex]!=' ' && str[tempindex]!=0 && str[tempindex]!='\n'){
      ++tempindex;
   }
   return(tempindex);
}
void wrap(char * s, const int wrapline){

   int index=0;
   int curlinelen = 0;
   while(s[index] != '\0'){

      if(s[index] == '\n'){
         curlinelen=0;
      }
      else if(s[index] == ' '){

         if(curlinelen+wordlen(&s[index+1]) >= wrapline){
            s[index] = '\n';
            curlinelen = 0;
         }

      }

      curlinelen++;
      index++;
   }

}

int main(void)
{
    const u32 clear_color = RGBA8(255,255,255,255);
    const u32 cornflower_blue = RGBA8(100,149,237,255);
    const u32 select_gray = RGBA8(176,196,222,255);
    const u32 text_color = RGBA8(32,32,32,255);
    const u32 shuffle_green = RGBA8(152,251,152, 127);
    gfxInitDefault();
    cfguInit();
    srvInit();  
    hidInit();
    fsInit();   
    ptmSysmInit();
    prepare_archives();
    unzip_themes();
    int theme_count = get_number_entries("/Themes");
    theme_data **themes_list = calloc(theme_count, sizeof(theme_data));
    prepare_themes(themes_list);
    int splash_count = get_number_entries("/Splashes");
    u16 *splashes_list = calloc(splash_count, PATH_LENGTH);
    prepare_splashes(splashes_list);
    
    gfxInitDefault();
    gfxSet3D(false);
    screen_init();

    int top = 0;
    int pos = 0;
    const int select_size = 50;
    bool change = true;
    u32 kDown = hidKeysDown();
    int select_count = 0;
    bool splash_mode = false;
    
    while (aptMainLoop())
    {
        hidScanInput();
        kDown = hidKeysDown();
        if (kDown & KEY_DOWN)
        {
            if (pos < 3) pos++;
            else if (top + pos < theme_count - 1) top++;
            change = true;
        }

        if (kDown & KEY_UP)
        {
            if (pos > 0) pos--;
            else if (top > 0) top--;
            change = true;
        }

        if (kDown & KEY_A)
        {
            screen_begin_frame();
            screen_select(GFX_TOP);
            screen_clear(GFX_TOP, clear_color);
            screen_draw_rect(20, 20, 360, 200, cornflower_blue);
            if (!splash_mode) screen_draw_string(25, 25, 0.7f, 0.7f, text_color, "Installing theme(s)...");
            else screen_draw_string(25, 25, 0.7f, 0.7f, text_color, "Installing splash...");
            screen_end_frame();
            if (splash_mode) install_splash(&splashes_list[(pos + top) * PATH_LENGTH/sizeof(u16)]);
            else if (select_count > 0) shuffle_install(themes_list, theme_count); else themeInstall(*themes_list[pos + top]);
            screen_begin_frame();
            screen_select(GFX_TOP);
            screen_clear(GFX_TOP, clear_color);
            screen_draw_rect(20, 20, 360, 200, cornflower_blue);
            if (!splash_mode) screen_draw_string(25, 25, 0.7f, 0.7f, text_color, "Installing theme(s)...");
            else screen_draw_string(25, 25, 0.7f, 0.7f, text_color, "Installing splash...");
            screen_draw_string(25, 50, 0.7f, 0.7f, text_color, "Done!");
            screen_end_frame();
        }

        if (kDown & KEY_X)
        {
            bool current_select = themes_list[top + pos]->selected;
            if (current_select) select_count--;
            if (!current_select) select_count++;
            themes_list[top + pos]->selected = !current_select;
            change = true;
        }

        // if (kDown & KEY_Y)
        // {
        //     screen_begin_frame();
        //     screen_select(GFX_TOP);
        //     char preview_path[524];
        //     strcpy(preview_path, "sdmc:");
        //     straucat(preview_path, themes_list[top + pos]->path);
        //     strcat(preview_path, "/Preview.png");
        //     screen_load_texture_file(THEME_PREVIEW_TEXT, preview_path, true);
        //     screen_draw_texture(THEME_PREVIEW_TEXT, 0, 0);
        //     screen_select(GFX_BOTTOM);
        //     screen_draw_texture(THEME_PREVIEW_TEXT, -40, -240);
        //     screen_end_frame();
        //     hidScanInput();
        //     kDown = hidKeysDown();
        //     while (!(kDown & KEY_B))
        //     {
        //         hidScanInput();
        //         kDown = hidKeysDown();
        //     }
        //     screen_unload_texture(THEME_PREVIEW_TEXT);
        //     change = true;
        // }

        if (kDown & KEY_START)
        {
            closeThemeArchives();   
            PTMSYSM_ShutdownAsync(0);
            ptmSysmExit();
        }

        if (kDown & KEY_L)
        {
            splash_mode = !splash_mode;
            change = true;
        }

        screen_begin_frame();
        if (!change) goto end;
        if (splash_mode) goto splash;
        screen_select(GFX_TOP);
        screen_clear(GFX_TOP, clear_color);
        screen_draw_rect(20, 20, 360, 200, cornflower_blue);
        char title[87];
        char theme_name[0x40] = {0};
        wtoa(theme_name, themes_list[top + pos]->title);
        sprintf(title, "Title: %s\n", theme_name);
        wrap(title, 20);
        int height = screen_get_string_height(title, 0.6f, 0.6f);
        screen_draw_string(210, 23, 0.6f, 0.6f, text_color, title);
        char description[0x80 + 6];
        char theme_desc[0x80] = {0};
        wtoa(theme_desc, themes_list[top + pos]->description);
        sprintf(description, "Desc: %s\n", theme_desc);
        wrap(description, 23);
        screen_draw_string(210, 23 + height, 0.5f, 0.5f, text_color, description);
        height += screen_get_string_height(description, 0.5f, 0.5f);
        char author[0x40 + 6];
        char theme_auth[0x40] = {0};
        wtoa(theme_auth, themes_list[top + pos]->author);
        sprintf(author, "Auth: %s", theme_auth);
        wrap(author, 23);
        screen_draw_string(210, 23 + height, 0.5f, 0.5f, text_color, author);
        screen_draw_string(23, 23, 0.8f, 0.8f, text_color, "Press Y\nfor preview");
        screen_select(GFX_BOTTOM);
        screen_clear(GFX_BOTTOM, clear_color);
        screen_draw_rect(20, 20, 280, 200, cornflower_blue);
        screen_draw_rect(20, 20 + (select_size * pos), 280, select_size, select_gray);
        int y_text_pos = 37;
        for (int i = 0; i < 4; i++)
        {
            if (i + top == theme_count) 
            {
                break;
            }
            if (themes_list[i + top]->selected) screen_draw_rect(20, 20 + (select_size * i), 280, select_size, shuffle_green);
            char title[0x40] = {0};
            char print_title[0x40] = {0};
            wtoa(title, themes_list[i + top]->title);
            float width = screen_get_string_width(title, 0.6f, 0.6f);
            if (width > 210)
            {
                memcpy(print_title, title, 0x14);
                strcat(print_title, "...");

            } else strcpy(print_title, title);
            screen_draw_string(70, y_text_pos, 0.6f, 0.6f, text_color, print_title);
            y_text_pos += 50;
        }
        goto end;

        splash:
        screen_select(GFX_TOP);
        screen_clear(GFX_TOP, clear_color);
        screen_draw_rect(20, 20, 360, 200, cornflower_blue);

        screen_select(GFX_BOTTOM);
        screen_clear(GFX_BOTTOM, clear_color);
        screen_draw_rect(20, 20, 280, 200, cornflower_blue);
        screen_draw_rect(20, 20 + (select_size * pos), 280, select_size, select_gray);

        int y_text_pos_splash = 37;
        for (int i = 0; i < 4; i++)
        {
            if (i + top == splash_count) 
            {
                break;
            }
            char path[524] = {0};
            char print_path[0x40] = {0};
            wtoa(path, &splashes_list[(i + top) * PATH_LENGTH/sizeof(u16)]);
            float width = screen_get_string_width(&path[10], 0.6f, 0.6f);
            if (width > 210)
            {
                memcpy(print_path, &path[10], 0x14);
                strcat(print_path, "...");

            } else strcpy(print_path, &path[10]);
            screen_draw_string(70, y_text_pos_splash, 0.6f, 0.6f, text_color, print_path);
            y_text_pos_splash += 50;
        }

        end:
        screen_end_frame();
        change = false;
    }

    gfxExit();
    return 0;
}
