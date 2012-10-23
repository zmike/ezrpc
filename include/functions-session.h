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
#ifndef Z_SESSIONS_H
#define Z_SESSIONS_H

#include "zentific.h"
#include "functions.h"
#include "functions-zentific.h"

Eina_Bool _checkSessionUUID(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _checkSession(Zentific_Ctx *ctx);
Eina_Bool _getSession(Zentific_Ctx *ctx, const char *session_id);
Eina_Bool _removeSession(Zentific_Ctx *ctx, const char *session_id);
Eina_Bool _setLastLogin(Zentific_Ctx *ctx, int uid);
Eina_Bool _deauthSession(Zentific_Ctx *ctx, const char *session_id);
Eina_Bool _setSession(Zentific_Ctx *ctx);
Eina_Bool session_pre_call(Zentific_Ctx *ctx, Azy_Net *to_client);
#endif
