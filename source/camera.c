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
#include "draw.h"
#include "fs.h"

void init_qr(void)
{
	camInit();
	CAMU_SetSize(SELECT_OUT1, SIZE_CTR_TOP_LCD, CONTEXT_A);
	CAMU_SetOutputFormat(SELECT_OUT1, OUTPUT_RGB_565, CONTEXT_A);

	CAMU_SetNoiseFilter(SELECT_OUT1, true);
	CAMU_SetAutoExposure(SELECT_OUT1, true);
	CAMU_SetAutoWhiteBalance(SELECT_OUT1, true);

	CAMU_SetTrimming(PORT_CAM1, false);

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

void take_picture(void)
{
	u32 transfer_size;
	Handle cam_handle = 0;
	CAMU_GetMaxBytes(&transfer_size, 400, 240);
	CAMU_SetTransferBytes(PORT_CAM1, transfer_size, 400, 240);
	CAMU_Activate(SELECT_OUT1);
	CAMU_ClearBuffer(PORT_CAM1);
	CAMU_StartCapture(PORT_CAM1);
	CAMU_SetReceiving(&cam_handle, buf, PORT_CAM1, 400 * 240 * 2, transfer_size);
	svcWaitSynchronization(cam_handle, U64_MAX);
	CAMU_StopCapture(PORT_CAM1);
	svcCloseHandle(cam_handle);
	CAMU_Activate(PORT_NONE);
}

/*
Putting this in camera because I'm too lazy to make a network.c
This'll probably get refactored later
*/
Result http_get(char *url, char *path)
{
	Result ret;
	httpcContext context;
	char *new_url = NULL;
	u32 status_code;
	u32 content_size = 0;
	u32 read_size = 0;
	u32 size = 0;
	u8 *buf;
	u8 *last_buf;

	do {
		ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
		ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify); // should let us do https
		ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
		ret = httpcAddRequestHeaderField(&context, "User-Agent", "Anemone3DS/1.1.0");
		ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");

		ret = httpcBeginRequest(&context);
		if (ret != 0)
		{
			httpcCloseContext(&context);
			if (new_url != NULL) free(new_url);
			return ret;
		}

		ret = httpcGetResponseStatusCode(&context, &status_code);
		if(ret!=0){
			httpcCloseContext(&context);
			if(new_url!=NULL) free(new_url);
			return ret;
		}

		if ((status_code >= 301 && status_code <= 303) || (status_code >= 307 && status_code <= 308))
		{
			if (new_url == NULL) new_url = malloc(0x1000);
			ret = httpcGetResponseHeader(&context, "Location", new_url, 0x1000);
			url = new_url;
			httpcCloseContext(&context);
		}
	} while ((status_code >= 301 && status_code <= 303) || (status_code >= 307 && status_code <= 308));

	if (status_code != 200)
	{
		httpcCloseContext(&context);
		if (new_url != NULL) free(new_url);
		return ret;
	}

	ret = httpcGetDownloadSizeState(&context, NULL, &content_size);
	if (ret != 0)
	{
		httpcCloseContext(&context);
		if (new_url != NULL) free(new_url);
		return ret;
	}

	buf = malloc(0x1000);
	if (buf == NULL)
	{
		httpcCloseContext(&context);
		free(new_url);
		return -2;
	}

	char *content_disposition = malloc(1024);
	ret = httpcGetResponseHeader(&context, "Content-Disposition", content_disposition, 1024);
	if (ret != 0)
	{
		free(content_disposition);
		free(new_url);
		free(buf);
	}

	char *filename;
	filename = strtok(content_disposition, "\"");
	filename = strtok(NULL, "\"");

	do {
		ret = httpcDownloadData(&context, buf + size, 0x1000, &read_size);
		size += read_size;

		if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
		{
			last_buf = buf;
			buf = realloc(buf, size + 0x1000);
			if (buf == NULL)
			{
				httpcCloseContext(&context);
				free(last_buf);
				return ret;
			}
		}
	} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

	last_buf = buf;
	buf = realloc(buf, size);
	if (buf == NULL)
	{
		httpcCloseContext(&context);
		free(last_buf);
		return -1;
	}

	char path_to_file[0x106] = {0};
	strcpy(path_to_file, path);
	strcat(path_to_file, filename);
	remake_file(path_to_file, ArchiveSD, size);
	buf_to_file(size, path_to_file, ArchiveSD, (char*)buf);

	return 0;
}