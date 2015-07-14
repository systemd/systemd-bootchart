#ifndef foosdmessageshfoo
#define foosdmessageshfoo

/***
  This file is part of systemd.

  Copyright 2012 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <systemd/sd-id128.h>

#include "_sd-common.h"

_SD_BEGIN_DECLARATIONS;

#define SD_MESSAGE_INVALID_CONFIGURATION SD_ID128_MAKE(c7,72,d2,4e,9a,88,4c,be,b9,ea,12,62,5c,30,6c,01)
#define SD_MESSAGE_BOOTCHART             SD_ID128_MAKE(9f,26,aa,56,2c,f4,40,c2,b1,6c,77,3d,04,79,b5,18)

_SD_END_DECLARATIONS;

#endif
