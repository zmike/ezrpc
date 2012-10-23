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
#ifndef Z_GROUP_H
#define Z_GROUP_H

#include "ztypes.h"

Eina_Bool _isUserGroup(Zentific_Ctx *ctx, int gid);
Eina_Bool _modifyUserGroup(Zentific_Ctx *ctx, int gid, USER_GROUP *ug);
Eina_Bool _removeUserGroup(Zentific_Ctx *ctx, int gid);
Eina_Bool _addToUserGroup(Zentific_Ctx *ctx, int uid, int gid);
Eina_Bool _removeFromUserGroup(Zentific_Ctx *ctx, int uid, int gid);
Eina_Bool _renameUserGroup(Zentific_Ctx *ctx, int gid, const char *new_name);
Eina_Bool _isUserGroupMember(Zentific_Ctx *ctx, int uid, int gid);
Eina_Bool _getUserGroup(Zentific_Ctx *ctx, int gid);
Eina_Bool _getUserGroupMembers(Zentific_Ctx *ctx, int gid);
Eina_Bool _getUserGroupMembersFull(Zentific_Ctx *ctx, int gid);
Eina_Bool _isUserGroupByName(Zentific_Ctx *ctx, const char *name);
Eina_Bool _modifyUserGroupByName(Zentific_Ctx *ctx, const char *groupname, USER_GROUP *ug);
Eina_Bool _removeUserGroupByName(Zentific_Ctx *ctx, const char *groupname);
Eina_Bool _renameUserGroupByName(Zentific_Ctx *ctx, const char *groupname, const char *new_name);
Eina_Bool _getUserGroupByName(Zentific_Ctx *ctx, const char *groupname);
Eina_Bool _getUserGroupMembersByName(Zentific_Ctx *ctx, const char *groupname);
Eina_Bool _isUserGroupMemberByName(Zentific_Ctx *ctx, int uid, const char *groupname);
Eina_Bool _addToUserGroupByName(Zentific_Ctx *ctx, int uid, const char *groupname);
Eina_Bool _removeFromUserGroupByName(Zentific_Ctx *ctx, int uid, const char *groupname);
Eina_Bool _isUserGroupMemberByNameAndUsername(Zentific_Ctx *ctx, const char *username, const char *groupname);
Eina_Bool _addToUserGroupByNameAndUsername(Zentific_Ctx *ctx, const char *username, const char *groupname);
Eina_Bool _removeFromUserGroupByNameAndUsername(Zentific_Ctx *ctx, const char *username, const char *groupname);
Eina_Bool _addUserGroup(Zentific_Ctx *ctx, USER_GROUP *ug);
Eina_Bool _getAllUserGroups(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getAllUserGroupNames(Zentific_Ctx *ctx, int offset, int recordcount);
Eina_Bool _getAllUserGroupsFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount);
Eina_Bool _getUserGroupsByUID(Zentific_Ctx *ctx, int uid);
Eina_Bool _getUserGroupsByName(Zentific_Ctx *ctx, const char *username);
Eina_Bool _getUserGroupNamesByUID(Zentific_Ctx *ctx, int uid);
Eina_Bool _getUserGroupNamesByName(Zentific_Ctx *ctx, const char *username);
Eina_Bool _getUserGroupName(Zentific_Ctx *ctx, int gid);
Eina_Bool _getUserGroupId(Zentific_Ctx *ctx, const char *groupname);
Eina_Bool _getUserGroupCount(Zentific_Ctx *ctx);
#endif
