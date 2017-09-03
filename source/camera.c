/*
*   This file is part of Anemone3DS
*   Copyright (C) 2016-2017 Alex Taber ("astronautlevel"), Dawid Eckert ("daedreth")
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

#include "camera.h"

#include "quirc/quirc.h"
#include "pp2d/pp2d/pp2d.h"
#include "draw.h"

void init_qr(void)
{
	qr_mode = true;

	camInit();
	CAMU_SetSize(SELECT_OUT1, SIZE_CTR_TOP_LCD, CONTEXT_A);
	CAMU_SetOutputFormat(SELECT_OUT1, OUTPUT_RGB_565, CONTEXT_A);

	CAMU_SetNoiseFilter(SELECT_OUT1, true);
	CAMU_SetAutoExposure(SELECT_OUT1, true);
	CAMU_SetAutoWhiteBalance(SELECT_OUT1, true);
	CAMU_SetFrameRate(SELECT_OUT1, FRAME_RATE_30);

	CAMU_SetTrimming(PORT_CAM1, true);
	CAMU_SetTrimmingParamsCenter(PORT_CAM1, 400, 240, 400, 240);
	CAMU_GetMaxBytes(&transfer_size, 400, 240);
	CAMU_SetTransferBytes(PORT_CAM1, transfer_size, 400, 240);
	CAMU_ClearBuffer(PORT_CAM1);
	CAMU_SetReceiving(&cam_handle, buf, PORT_CAM1, 400 * 240 * 2, transfer_size);
	CAMU_StartCapture(PORT_CAM1);

	context = quirc_new();
	quirc_resize(context, 400, 240);
}

void exit_qr(void)
{
	CAMU_Activate(SELECT_NONE);
	camExit();
	quirc_destroy(context);
}

void take_picture(void)
{
	pp2d_begin_draw(GFX_TOP);
	pp2d_free_texture(TEXTURE_QR);
	// svcWaitSynchronization(cam_handle, U64_MAX);
	u32 rgba8_buf[240 * 400];
	for (int i = 0; i < 240 * 400; i++)
	{
		rgba8_buf[i] = RGB565_TO_RGBA8(buf[i]);
	}
	pp2d_load_texture_memory(TEXTURE_QR, rgba8_buf, 400, 240);
	pp2d_draw_texture(TEXTURE_QR, 0, 0);
	pp2d_end_draw();
}