#pragma once

#include "minizip/unzip.h"

#define PATH_LENGTH 524

Result prepare_splashes(u16*);
Result unzip_splashes();
Result install_splash(u16 *);