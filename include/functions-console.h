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
#ifndef Z_CONSOLE_H
#define Z_CONSOLE_H

#include "zentific.h"
#include "functions.h"
#include "sys/wait.h"

Eina_Bool _saveGraphicalConsolePort(Zentific_Ctx *ctx, const char *uuid, int tunnelport, int externalport);
Eina_Bool _getGraphicalConsoleTunnelPort(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _getGraphicalConsolePort(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _saveTextConsolePort(Zentific_Ctx *ctx, const char *uuid, int externalport);
Eina_Bool _getTextConsolePort(Zentific_Ctx *ctx, const char *uuid);
Eina_Bool _connectTextConsole(Zentific_Ctx *ctx, SESSION *session, const char *uuid);
Eina_Bool _connectGraphicalConsole(Zentific_Ctx *ctx, SESSION *session, const char *uuid);
Eina_Bool _disconnectConsole(Zentific_Ctx *ctx, const char *uuid, const char *type);
Eina_Bool _getConsoleLockSession(Zentific_Ctx *ctx, const char *uuid, const char *type);
Eina_Bool _getConsoleClientHost(Zentific_Ctx *ctx, const char *uuid, const char *type);
Eina_Bool _getConsoleNodeHost(Zentific_Ctx *ctx, const char *uuid, const char *type);
Eina_Bool _getConsoleLockUser(Zentific_Ctx *ctx, const char *uuid, const char *type);
Eina_Bool _hasConsoleLock(Zentific_Ctx *ctx, const char *uuid, const char *type);
Eina_Bool _acquireConsoleLock(Zentific_Ctx *ctx, const char *uuid, SESSION *session, const char *type);
Eina_Bool _releaseConsoleLock(Zentific_Ctx *ctx, const char *uuid, const char *sessionid, const char *type);
Eina_Bool _getConsoleLockVmsBySession(Zentific_Ctx *ctx, const char *session_id);
Eina_Bool _getConsoleType(Zentific_Ctx *ctx, const char *uuid, const char *session_id);
Eina_Bool _disconnectConsolesBySession(Zentific_Ctx *ctx, const char *session_id);
Eina_Bool _getSupportedConsoles(Zentific_Ctx *ctx, const char *uuid);
#endif
