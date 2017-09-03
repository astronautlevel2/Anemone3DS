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

	buf = malloc(sizeof(u16) * 400 * 240);

	context = quirc_new();
	quirc_resize(context, 400, 240);
}

void exit_qr(void)
{
	CAMU_Activate(SELECT_NONE);
	camExit();
	quirc_destroy(context);
	free(buf);
}