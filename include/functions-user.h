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
#ifndef Z_USER_H
#define Z_USER_H

#include "ztypes.h"

Eina_Bool _isUserUID(Zentific_Ctx *ctx, int uid);
Eina_Bool _isActive(Zentific_Ctx *ctx, int uid);
Eina_Bool _modifyUser(Zentific_Ctx *ctx, int uid, USER *u);
Eina_Bool _getUser(Zentific_Ctx *ctx, int uid);
Eina_Bool _getUserType(Zentific_Ctx *ctx, int uid);
Eina_Bool _isSuperUser(Zentific_Ctx *ctx, int uid);
Eina_Bool _removeUser(Zentific_Ctx *ctx, int uid);
Eina_Bool _setPassword(Zentific_Ctx *ctx, int uid, const char *pass);
Eina_Bool _getUsername(Zentific_Ctx *ctx, int uid);
Eina_Bool _generatePassword(Zentific_Ctx *ctx, int uid);
Eina_Bool _setUserAttribute(Zentific_Ctx *ctx, int uid, const char *attribute, const char *value);
Eina_Bool _logoutByUID(Zentific_Ctx *ctx, int uid);
Eina_Bool _isUserName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _isActiveByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _modifyUserByName(Zentific_Ctx *ctx, const char *name, USER *u);
Eina_Bool _removeUserByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _setPasswordByName(Zentific_Ctx *ctx, const char *name, const char *pass);
Eina_Bool _generatePasswordByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _setUserAttributeByName(Zentific_Ctx *ctx, const char *name, const char *attribute, const char *value);
Eina_Bool _logoutByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getUserByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getUserTypeByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _isSuperUserByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getActiveUserUID(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getUserUID(Zentific_Ctx *ctx, const char *name);
Eina_Bool _getLanguageCode(Zentific_Ctx *ctx, int id);
Eina_Bool _addUser(Zentific_Ctx *ctx, USER *user);
Eina_Bool _logout(Zentific_Ctx *ctx, const char *session_id);
Eina_Bool _getAllUsers(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getAllUsersFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getLoggedInUsers(Zentific_Ctx *ctx);
Eina_Bool _getLoggedInUsernames(Zentific_Ctx *ctx);
Eina_Bool _getUserCount(Zentific_Ctx *ctx);
#endif
