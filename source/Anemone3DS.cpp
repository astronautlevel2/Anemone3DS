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

#include "Anemone3DS.h"

#include "menus/theme_menu.h"
#include "menus/splash_menu.h"
#include "menus/badge_menu.h"

#include "menus/remote_theme_menu.h"
#include "menus/remote_splash_menu.h"

#include "draw.h"
#include "file.h"

static Handle scroll_lock;
static Handle scroll_start_event, scroll_stop_thread;
// Used by the scroll thread to tell to the main thread the icons can be drawn safely
static Handle scroll_ready_to_draw_event;

void scroll_thread_function_external(void* void_arg)
{
    Anemone3DS* this_ = static_cast<Anemone3DS*>(void_arg);
    Handle handles[2] = {
        scroll_start_event,
        scroll_stop_thread,
    };

    s32 index;
    while(true)
    {
        index = -1;
        svcWaitSynchronizationN(&index, handles, 2, false, U64_MAX);

        if(index == 0)
        {
            svcWaitSynchronization(scroll_lock, U64_MAX);
            this_->scroll_thread_function();
            svcReleaseMutex(scroll_lock);
        }
        else
        {
            break;
        }
    }
}

void Anemone3DS::scroll_thread_function()
{
    this->menus[this->selected_menu]->scroll_icons(&scroll_ready_to_draw_event);
}

void Anemone3DS::reload_menu(MenuType menu)
{
    if(menu < MODES_AMOUNT)
        this->menus[menu] = nullptr;  // Since we're low on linear (and regular) heap space, free up the menu before recreating it

    std::unique_ptr<Menu> new_menu;
    switch(menu)
    {
        case MODE_THEMES:
            new_menu = std::make_unique<ThemeMenu>();
            break;
        case MODE_SPLASHES:
            new_menu = std::make_unique<SplashMenu>();
            break;
        case MODE_BADGES:
            new_menu = std::make_unique<BadgeMenu>();
            break;
        default:
            svcBreak(USERBREAK_PANIC);
            break;
    }
    this->menus[menu] = std::move(new_menu);
}

void Anemone3DS::select_previous_menu()
{
    if(this->selected_menu == 0)
        this->selected_menu = this->menus.size();
    --this->selected_menu;
    this->set_menu();
}

void Anemone3DS::select_next_menu()
{
    if(++(this->selected_menu) == this->menus.size())
        this->selected_menu = 0;
    this->set_menu();
}

void Anemone3DS::select_menu(MenuType menu)
{
    this->selected_menu = menu;
    this->set_menu();
}

void Anemone3DS::set_menu()
{
    this->current_menu = this->menus[this->selected_menu].get();
}

void Anemone3DS::enter_browser_mode()
{
    std::unique_ptr<RemoteMenu> new_browser_menu;
    switch(this->selected_menu)
    {
        case MODE_THEMES:
            new_browser_menu = std::make_unique<RemoteThemeMenu>();
            break;
        case MODE_SPLASHES:
            new_browser_menu = std::make_unique<RemoteSplashMenu>();
            break;
        case MODE_BADGES:
            draw_error(ERROR_LEVEL_WARNING, ERROR_TYPE_THEMEPLAZA_BADGES_DISABLED);
            break;
        default:
            svcBreak(USERBREAK_PANIC);
            break;
    }

    if(new_browser_menu && new_browser_menu->ready)
    {
        this->downloaded_any = false;
        this->browser_menu = std::move(new_browser_menu);
        this->current_menu = this->browser_menu.get();
    }
}

void Anemone3DS::enter_qr_mode()
{
    std::unique_ptr<QrMenu> new_qr_menu = std::make_unique<QrMenu>();
    if(new_qr_menu->ready)
    {
        this->qr_menu = std::move(new_qr_menu);
        this->current_menu = this->qr_menu.get();
    }
}

void Anemone3DS::enter_list_mode()
{
    if(this->browser_menu)
    {
        this->browser_menu = nullptr;
        if(this->downloaded_any)
            this->reload_menu(static_cast<MenuType>(this->selected_menu));
    }
    else if(this->qr_menu)
    {
        auto downloaded_any_modes = this->qr_menu->downloaded_any;
        this->qr_menu = nullptr;
        for(size_t i = 0; i < MODES_AMOUNT; i++)
        {
            if(downloaded_any_modes[i])
                this->reload_menu(static_cast<MenuType>(i));
        }
    }
    else
        svcBreak(USERBREAK_PANIC);

    this->set_menu();
}

void Anemone3DS::move_schedule_sleep()
{
    this->sleep_scheduled = true;
}

void Anemone3DS::installed_a_theme()
{
    this->installed_theme = true;
}

void Anemone3DS::downloaded_from_tp()
{
    this->downloaded_any = true;
}

void Anemone3DS::handle_action_return(MenuActionReturn action_result)
{
    static std::array<std::function<void()>, MENU_ACTION_AMOUNT> actions{
        std::bind(&Anemone3DS::select_previous_menu, this),
        std::bind(&Anemone3DS::select_next_menu, this),
        std::bind(&Anemone3DS::select_menu, this, MODE_THEMES),
        std::bind(&Anemone3DS::select_menu, this, MODE_SPLASHES),
        std::bind(&Anemone3DS::select_menu, this, MODE_BADGES),

        std::bind(&Anemone3DS::enter_browser_mode, this),
        std::bind(&Anemone3DS::enter_qr_mode, this),
        std::bind(&Anemone3DS::enter_list_mode, this),

        std::bind(&Anemone3DS::move_schedule_sleep, this),
        std::bind(&Anemone3DS::installed_a_theme, this),
        std::bind(&Anemone3DS::downloaded_from_tp, this),
    };

    actions[action_result-1]();
}

void Anemone3DS::init_services()
{
    consoleDebugInit(debugDevice_SVC);
    cfguInit();
    ptmuInit();
    acInit();
    have_sound = R_SUCCEEDED(ndspInit());
    APT_GetAppCpuTimeLimit(&this->old_time_limit);
    APT_SetAppCpuTimeLimit(30);
    // aptSetHomeAllowed(false);
    httpcInit(0);
}

void Anemone3DS::init_menus()
{
    this->menus[MODE_THEMES] = std::make_unique<ThemeMenu>();
    this->menus[MODE_SPLASHES] = std::make_unique<SplashMenu>();
    this->menus[MODE_BADGES] = std::make_unique<BadgeMenu>();
}

void Anemone3DS::init_threads()
{
    this->scroll_thread = NULL;
    for(const auto& menu : this->menus)
    {
        if(menu->needs_thread())
        {
            DEBUG("Thread needed!\n");
            s32 prio = 0;
            svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
            this->scroll_thread = threadCreate(scroll_thread_function_external, this, 32*1024, prio-1, -2, false);
            return;
        }
    }
    DEBUG("Thread not needed!\n");
}

void Anemone3DS::exit_threads()
{
    if(this->scroll_thread)
    {
        svcSignalEvent(scroll_stop_thread);
        threadJoin(this->scroll_thread, U64_MAX);
        threadFree(this->scroll_thread);
        this->scroll_thread = NULL;
    }
}

void Anemone3DS::exit_menus()
{
    this->qr_menu = nullptr;
    this->browser_menu = nullptr;

    for(auto& menu : this->menus)
        menu = nullptr;
}

void Anemone3DS::exit_services()
{
    httpcExit();
    if (old_time_limit != UINT32_MAX)
        APT_SetAppCpuTimeLimit(this->old_time_limit);
    if(have_sound)
        ndspExit();
    acExit();
    ptmuExit();
    cfguExit();
}

Anemone3DS::Anemone3DS()
{
    this->installed_theme = false;
    if(envIsHomebrew())
    {
        s64 out;
        svcGetSystemInfo(&out, 0x10000, 0);
        running_from_hax = !out;
        Handle hbldr_handle;
        have_luma = R_SUCCEEDED(svcConnectToPort(&hbldr_handle, "hb:ldr"));
        svcCloseHandle(hbldr_handle);
    }

    this->init_services();

    DEBUG("Have luma: %s\n", have_luma ? "Yes" : "No");
    DEBUG("Running under *hax: %s\n", running_from_hax ? "Yes" : "No");

    open_archives();
    init_screens();

    start_frame(-1);
    draw_basic_interface();
    end_frame();

    if(R_FAILED(theme_result))
        return;

    if(R_FAILED(badge_result))
    {
        DEBUG("No badge extdata...\n");
        draw_error(ERROR_LEVEL_WARNING, ERROR_TYPE_NO_BADGE_EXTDATA);
    }

    if(!have_luma)
    {
        DEBUG("No luma...\n");
        draw_error(ERROR_LEVEL_WARNING, ERROR_TYPE_NO_LUMA);
    }

    svcCreateMutex(&scroll_lock, true);
    svcCreateEvent(&scroll_ready_to_draw_event, RESET_ONESHOT);
    svcCreateEvent(&scroll_start_event, RESET_ONESHOT);
    svcCreateEvent(&scroll_stop_thread, RESET_ONESHOT);

    this->init_menus();
    this->init_threads();

    this->select_menu(MODE_THEMES);
}

Anemone3DS::~Anemone3DS()
{
    this->exit_threads();

    svcCloseHandle(scroll_lock);
    svcCloseHandle(scroll_ready_to_draw_event);
    svcCloseHandle(scroll_start_event);
    svcCloseHandle(scroll_stop_thread);

    this->exit_menus();
    exit_screens();

    close_archives();
    this->exit_services();

    if(this->installed_theme && !power_pressed)
    {
        if(running_from_hax)
        {
            APT_HardwareResetAsync();
        }
        else
        {
            srvPublishToSubscriber(0x202, 0);
        }
    }
}

void Anemone3DS::draw()
{
    start_frame(this->current_menu->background_color);

    if(this->current_menu->in_instructions)
    {
        static constexpr u32 instruction_fade_color = C2D_Color32f(0, 0, 0, 0.75f);
        C2D_Fade(instruction_fade_color);
    }

    this->current_menu->draw();

    if(this->current_menu->in_instructions)
    {
        C2D_Fade(0);
        this->current_menu->draw_instructions();
    }
    else if(!this->current_menu->in_preview())
    {
        if(!this->browser_menu)
        {
            switch_screen(GFX_BOTTOM);
            static constexpr float y = (BARS_SIZE - 30*0.6f)/2.0f - 1.0f;
            static const float l_width = get_text_width(TEXT_GENERAL, TEXT_L, 0.6f);
            static const float r_width = get_text_width(TEXT_GENERAL, TEXT_R, 0.6f);
            static constexpr float x_step = BARS_SIZE;
            float x = 8.0f;

            draw_text(TEXT_GENERAL, TEXT_L, COLOR_WHITE, x + (BARS_SIZE - l_width)/2.0f, y, 0.2f, 0.6f, 0.6f);
            x += x_step;
            TextID icon_text = TEXT_THEME_ICON;
            for(const auto& menu : this->menus)
            {
                C2D_DrawRectSolid(x, 0, 0.2f, x_step, BARS_SIZE, menu->background_color);
                draw_text(TEXT_GENERAL, icon_text, COLOR_WHITE, x + (BARS_SIZE - get_text_width(TEXT_GENERAL, icon_text, 0.6f))/2.0f, y, 0.2f, 0.6f, 0.6f);
                icon_text = static_cast<TextID>(icon_text + 1);
                x += x_step;
            }
            draw_text(TEXT_GENERAL, TEXT_R, COLOR_WHITE, x + (BARS_SIZE - r_width)/2.0f, y, 0.2f, 0.6f, 0.6f);
        }
    }

    end_frame();
}

void Anemone3DS::update()
{
    if(R_FAILED(theme_result))
    {
        DEBUG("No theme extdata...\n");
        draw_error(ERROR_LEVEL_CRITICAL, ERROR_TYPE_NO_THEME_EXTDATA);
        return;
    }

    // Only use the thread when it's created and needed
    this->current_menu->calculate_new_scroll();
    if(this->current_menu->should_scroll)
    {
        if(this->scroll_thread)
        {
            svcSignalEvent(scroll_start_event);
            svcReleaseMutex(scroll_lock);
            svcWaitSynchronization(scroll_ready_to_draw_event, U64_MAX);
        }
        else
        {
            this->menus[this->selected_menu]->scroll_icons(nullptr);
        }
    }

    if(this->qr_menu)
        this->qr_menu->scan();
    else
        this->draw();

    if(this->current_menu->should_scroll)
    {
        if(this->scroll_thread)
        {
            svcWaitSynchronization(scroll_lock, U64_MAX);
        }
        this->current_menu->should_scroll = false;
    }

    if(this->sleep_scheduled)
    {
        svcSleepThread(150 * 1000 * 1000);
        this->sleep_scheduled = false;
    }

    hidScanInput();

    u32 kDown = hidKeysDown();

    if(kDown & KEY_START)
    {
        power_pressed = false;
        running = false;
        return;
    }
    else if(kDown & KEY_SELECT && !this->current_menu->in_preview() && !this->qr_menu)
    {
        this->current_menu->toggle_instructions_mode();
        return;
    }

    const auto& current_actions = this->current_menu->current_actions.top();
    for(const auto& [keycombo, action] : current_actions.down)
    {
        if((keycombo & kDown) == keycombo)
        {
            MenuActionReturn result = action();
            if(result != RETURN_NONE)
                this->handle_action_return(result);
            return;
        }
    }

    u32 kHeld = hidKeysHeld();

    for(const auto& [keycombo, action] : current_actions.held)
    {
        if((keycombo & kHeld) == keycombo)
        {
            MenuActionReturn result = action();
            if(result != RETURN_NONE)
                this->handle_action_return(result);
            return;
        }
    }
}
