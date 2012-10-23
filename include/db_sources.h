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
#ifndef ZDB_SOURCES_H
#define ZDB_SOURCES_H

#include "ztypes.h"
Eina_Bool db_init(Zrpcdb_Connect_Cb cb, Ecore_Event_Handler_Cb error_cb, Ecore_Event_Handler_Cb result_cb, zentific_config *conf);
int db_uninit(DB *db);

#endif // SOURCES_H
