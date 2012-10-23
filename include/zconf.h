/* zrpc: RPC database+server for Zentific
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Zentific LLC and AUTHORS
 *
 * All rights reserved.
 * Use is subject to license terms.
 *
 * Please visit http://zentific.com for news and updates
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */
#ifndef Z_COMMON_H
#define Z_COMMON_H

#include "ztypes.h"

void config_reparse(zentific_config **config);
void config_free(zentific_config *config);
int config_parse(int line, const char *name, const char *value, zentific_config *config);
void config_defaults_fill(zentific_config *zconf);
void config_print(zentific_config *config);
zentific_config *config_new(void);

#endif
