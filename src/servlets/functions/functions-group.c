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
#include "functions.h"
#include "functions-user.h"
#include "functions-group.h"

static void
group_cb_modify(Zentific_Ctx *ctx, int ret){
	USER_GROUP *ug;
	MARK_CB;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	ug = CTX_PARAM_GET;
	if ((ret == -1) && ctx->error){ /* new name does not exist */
		ctx->error = 0;
		db->modifyUserGroup(db, ctx, ug->gid, ug);
		return;
	}
	if (!ctx->error)
		ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
	RESUME;
}

static void
group_cb_add(Zentific_Ctx *ctx, Eina_Bool ret){
	USER_GROUP *ug;
	MARK_CB;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	ug = CTX_PARAM_GET;
	if (!ret) {
		ctx->error = 0;
		db->addUserGroup(db, ctx, ug);
		return;
	}
	if (!ctx->error)
		ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
	RESUME;
}
/*********************************************************************************
 * GROUP FUNCTIONS BY GID
 */

Eina_Bool _isUserGroup(Zentific_Ctx *ctx, int gid){
	if (gid < 0){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	db->isUserGroup(db, ctx, gid);
	CTX_RETURN;
}

Eina_Bool _modifyUserGroup(Zentific_Ctx *ctx, int gid, USER_GROUP *ug){
	_getUserGroupId(ctx, ug->name);
	ug->gid = gid;
	CTX_CB_ADD(group_cb_modify);
	CTX_PARAM_ADD(group_cb_modify, ug);
	CTX_RETURN;
}

/* removes just the group and mappings to it from uids */
Eina_Bool _removeUserGroup(Zentific_Ctx *ctx, int gid){
	db->removeUserGroup(db, ctx, gid);
	CTX_RETURN;
}

Eina_Bool _addToUserGroup(Zentific_Ctx *ctx, int uid, int gid){
	if (!ctx->count) {
		_isUserUID(ctx, uid);
		CTX_CB_ADD(cb_bool_rewind);
	} else if (!ctx->error)
		db->addToUserGroup(db, ctx, uid, gid);
	CTX_RETURN;
}

Eina_Bool _removeFromUserGroup(Zentific_Ctx *ctx, int uid, int gid){
	db->removeFromUserGroup(db, ctx, uid, gid);
	CTX_RETURN;
}

Eina_Bool _renameUserGroup(Zentific_Ctx *ctx, int gid, const char *new_name){
	switch (ctx->count){
		case 0:
			_isUserGroupByName(ctx, new_name);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (ctx->error){
				if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){
					ctx->error = 0;
					db->renameUserGroup(db, ctx, gid, new_name);
				}
			} else
				ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
			break;
	}
	CTX_RETURN;
}

Eina_Bool _isUserGroupMember(Zentific_Ctx *ctx, int uid, int gid){
	db->isUserGroupMember(db, ctx, uid, gid);
	CTX_RETURN;
}

Eina_Bool _getUserGroup(Zentific_Ctx *ctx, int gid){
	db->getUserGroup(db, ctx, gid);
	CTX_RETURN;
}

/* returns list of uids */
Eina_Bool _getUserGroupMembers(Zentific_Ctx *ctx, int gid){
	db->getUserGroupMembers(db, ctx, gid);
	CTX_RETURN;
}

Eina_Bool _getUserGroupMembersFull(Zentific_Ctx *ctx, int gid){
	db->getUserGroupMembersFull(db, ctx, gid);
	CTX_RETURN;
}

/*********************************************************************************
 * GROUP FUNCTIONS BY GROUP NAME
 */

Eina_Bool _isUserGroupByName(Zentific_Ctx *ctx, const char *name){
	db->isUserGroupByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _modifyUserGroupByName(Zentific_Ctx *ctx, const char *groupname, USER_GROUP *ug){
	switch (ctx->count){
		case 0:
			_isUserGroupByName(ctx, ug->name);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (ctx->error){
				if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){
					ctx->error = 0;
					db->modifyUserGroupByName(db, ctx, groupname, ug);
				}
			} else
				ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
			break;
	}
	CTX_RETURN;
}

/* removes just the group and mappings to it from uids */
Eina_Bool _removeUserGroupByName(Zentific_Ctx *ctx, const char *groupname){
	db->removeUserGroupByName(db, ctx, groupname);
	CTX_RETURN;
}

Eina_Bool _renameUserGroupByName(Zentific_Ctx *ctx, const char *groupname, const char *new_name){
	switch (ctx->count){
		case 0:
			_isUserGroupByName(ctx, new_name);
			CTX_CB_ADD(cb_bool_rewind);
			break;
		default:
			if (ctx->error){
				if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE)){
					ctx->error = 0;
					db->renameUserGroupByName(db, ctx, groupname, new_name);
				}
			} else
				ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
			break;
	}
	CTX_RETURN;
}

Eina_Bool _getUserGroupByName(Zentific_Ctx *ctx, const char *groupname){
	db->getUserGroupByName(db, ctx, groupname);
	CTX_RETURN;
}

/* returns list of uids */
Eina_Bool _getUserGroupMembersByName(Zentific_Ctx *ctx, const char *groupname){
	db->getUserGroupMembersByName(db, ctx, groupname);
	CTX_RETURN;
}

Eina_Bool _isUserGroupMemberByName(Zentific_Ctx *ctx, int uid, const char *groupname){
	db->isUserGroupMemberByName(db, ctx, uid, groupname);
	CTX_RETURN;
}

Eina_Bool _addToUserGroupByName(Zentific_Ctx *ctx, int uid, const char *groupname){
	db->addToUserGroupByName(db, ctx, uid, groupname);
	CTX_RETURN;
}

Eina_Bool _removeFromUserGroupByName(Zentific_Ctx *ctx, int uid, const char *groupname){
	db->removeFromUserGroupByName(db, ctx, uid, groupname);
	CTX_RETURN;
}

Eina_Bool _isUserGroupMemberByNameAndUsername(Zentific_Ctx *ctx, const char *username, const char *groupname){
	db->isUserGroupMemberByNameAndUsername(db, ctx, username, groupname);
	CTX_RETURN;
}

Eina_Bool _addToUserGroupByNameAndUsername(Zentific_Ctx *ctx, const char *username, const char *groupname){
	db->addToUserGroupByNameAndUsername(db, ctx, username, groupname);
	CTX_RETURN;
}

Eina_Bool _removeFromUserGroupByNameAndUsername(Zentific_Ctx *ctx, const char *username, const char *groupname){
	db->removeFromUserGroupByNameAndUsername(db, ctx, username, groupname);
	CTX_RETURN;
}

/*********************************************************************************
 * GROUP FUNCTIONS MISC
 */

Eina_Bool _addUserGroup(Zentific_Ctx *ctx, USER_GROUP *ug){
	_isUserGroupByName(ctx, ug->name);
	CTX_CB_ADD(group_cb_add);
	CTX_PARAM_ADD(group_cb_add, ug);
	CTX_RETURN;
}

Eina_Bool _getAllUserGroups(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield && sortfield[0])
		db->getAllUserGroupsSorted(db, ctx, sortfield, desc, offset, recordcount);
	else
		db->getAllUserGroups(db, ctx, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllUserGroupNames(Zentific_Ctx *ctx, int offset, int recordcount){
	db->getAllUserGroupNames(db, ctx, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllUserGroupsFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield && sortfield[0])
		db->getAllUserGroupsFullSorted(db, ctx, sortfield, desc, offset, recordcount);
	else
		db->getAllUserGroupsFull(db, ctx, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getUserGroupsByUID(Zentific_Ctx *ctx, int uid){
	db->getUserGroupsByUID(db, ctx, uid);
	CTX_RETURN;
}

Eina_Bool _getUserGroupsByName(Zentific_Ctx *ctx, const char *username){
	db->getUserGroupsByName(db, ctx, username);
	CTX_RETURN;
}

Eina_Bool _getUserGroupNamesByUID(Zentific_Ctx *ctx, int uid){
	db->getUserGroupNamesByUID(db, ctx, uid);
	CTX_RETURN;
}

Eina_Bool _getUserGroupNamesByName(Zentific_Ctx *ctx, const char *username){
	db->getUserGroupNamesByName(db, ctx, username);
	CTX_RETURN;
}

Eina_Bool _getUserGroupName(Zentific_Ctx *ctx, int gid){
	db->getUserGroupName(db, ctx, gid);
	CTX_RETURN;
}

Eina_Bool _getUserGroupId(Zentific_Ctx *ctx, const char *groupname){
	db->getUserGroupId(db, ctx, groupname);
	CTX_RETURN;
}

Eina_Bool _getUserGroupCount(Zentific_Ctx *ctx){
	db->getUserGroupCount(db, ctx);
	CTX_RETURN;
}
