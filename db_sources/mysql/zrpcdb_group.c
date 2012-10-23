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
#include "zrpcdb.h"

/*********************************************************************************
 * GROUP FUNCTIONS BY GID
 */
Eina_Bool isUserGroup(DB *db, Zentific_Ctx *ctx, int gid){
	QIA(
		"SELECT `gid` FROM `groups` WHERE `gid`='%d'",
		gid);
}

Eina_Bool modifyUserGroup(DB *db, Zentific_Ctx *ctx, int gid, USER_GROUP *ug){
	QE(
		"UPDATE `groups`"
		" SET `name`='%s', `desc`='%s', `privilege`='%d'"
		" WHERE `gid`='%d'",
		ug->name, ug->desc, ug->privilege, gid);
}

Eina_Bool removeUserGroup(DB *db, Zentific_Ctx *ctx, int gid){
	QE(
		"DELETE FROM `groups` WHERE `gid`='%d'",
		gid);
}

Eina_Bool getUserGroup(DB *db, Zentific_Ctx *ctx, int gid){
	DB_VARS;
	QUERY(USER_GROUP_esql,
		"SELECT * FROM `groups` WHERE `gid`='%d'",
		gid);
	ctx->free_func = (Ecore_Cb)USER_GROUP_free;
	QID_RETURN;
}

Eina_Bool getUserGroupName(DB *db, Zentific_Ctx *ctx, int gid){
	QSA(
		"SELECT name FROM `groups` WHERE `gid`='%d'",
		gid);
}

Eina_Bool getUserGroupMembers(DB *db, Zentific_Ctx *ctx, int gid){
	QISA(
		"SELECT `uid` FROM `membership` WHERE `gid`='%d'",
		gid);
}

Eina_Bool getUserGroupMembersFull(DB *db, Zentific_Ctx *ctx, int gid){
	DB_VARS;
	QUERY(Array_USER_esql,
		"SELECT users.* FROM membership "
		"JOIN users ON membership.uid=users.uid "
		"WHERE gid='%d'",
		gid);
	ctx->free_func = (Ecore_Cb)Array_USER_free;
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

Eina_Bool addToUserGroup(DB *db, Zentific_Ctx *ctx, int uid, int gid){
	QE(
		"INSERT INTO `membership` (`uid`,`gid`) VALUES ('%d','%d')",
		uid, gid);
}

Eina_Bool removeFromUserGroup(DB *db, Zentific_Ctx *ctx, int uid, int gid){
	QE(
		"DELETE FROM `membership` WHERE `uid`='%d' AND `gid`='%d'",
		uid, gid);
}

Eina_Bool renameUserGroup(DB *db, Zentific_Ctx *ctx, int gid, const char *new_name){
	QE(
		"UPDATE `groups` SET `name`='%s' WHERE `gid`='%d'",
		new_name, gid);
}

Eina_Bool isUserGroupMember(DB *db, Zentific_Ctx *ctx, int uid, int gid){
	QIA(
		"SELECT `uid` FROM `membership` WHERE `uid`='%d' AND `gid`='%d'",
		uid, gid);
}
/*********************************************************************************
 * GROUP FUNCTIONS BY NAME
 */
Eina_Bool isUserGroupByName(DB *db, Zentific_Ctx *ctx, const char *name){
	QIA(
		"SELECT `gid` FROM `groups` WHERE `name`='%s'",
		name);
}

Eina_Bool modifyUserGroupByName(DB *db, Zentific_Ctx *ctx, const char *groupname, USER_GROUP *ug){
	QE(
		"UPDATE `groups`"
		" SET `name`='%s', `desc`='%s', `privilege`='%d' "
		"WHERE name = '%s'",
		ug->name, ug->desc, ug->privilege, groupname);
}

Eina_Bool removeUserGroupByName(DB *db, Zentific_Ctx *ctx, const char *groupname){
	QE(
		"DELETE FROM `groups` "
		"WHERE `gid`="
			"(SELECT gid FROM `groups` WHERE name = '%s' LIMIT 1)",
		groupname);
}

Eina_Bool getUserGroupByName(DB *db, Zentific_Ctx *ctx, const char *groupname){
	DB_VARS;
	QUERY(USER_GROUP_esql,
		"SELECT * FROM `groups` "
		"WHERE `gid` = "
			"(SELECT gid FROM `groups` WHERE name = '%s' LIMIT 1)",
		groupname);
	ctx->free_func = (Ecore_Cb)USER_GROUP_free;
	QID_RETURN;
}

Eina_Bool getUserGroupId(DB *db, Zentific_Ctx *ctx, const char *groupname){
	QIA(
		"SELECT gid FROM `groups` WHERE `name`='%s'",
		groupname);
}

Eina_Bool getUserGroupMembersByName(DB *db, Zentific_Ctx *ctx, const char *groupname){
	QISA(
		"SELECT `uid` FROM `membership` "
		"WHERE `gid` = "
			"(SELECT gid FROM `groups` WHERE name = '%s' LIMIT 1)",
		groupname);
}

Eina_Bool renameUserGroupByName(DB *db, Zentific_Ctx *ctx, const char *groupname, const char *new_name){
	QE(
		"UPDATE `groups` SET `name`='%s' "
		"WHERE `gid` = "
			"(SELECT gid FROM `groups` WHERE name = '%s' LIMIT 1)",
		new_name, groupname);
}

Eina_Bool addToUserGroupByName(DB *db, Zentific_Ctx *ctx, int uid, const char *groupname){
	QE(
		"INSERT INTO `membership` (`uid`,`gid`) VALUES "
		"("
			"'%d',"
			"(SELECT gid FROM `groups` WHERE name = '%s' LIMIT 1)"
		")",
		uid, groupname);
}

Eina_Bool removeFromUserGroupByName(DB *db, Zentific_Ctx *ctx, int uid, const char *groupname){
	QE(
		"DELETE FROM `membership` WHERE `uid`='%d' AND `gid`='%s'",
		uid, groupname);
}

Eina_Bool isUserGroupMemberByName(DB *db, Zentific_Ctx *ctx, int uid, const char *groupname){
	QIAB(
		"SELECT `uid` FROM `membership` "
		"WHERE `uid`='%d' AND `gid` = "
			"(SELECT gid FROM `groups` WHERE name = '%s' LIMIT 1)",
		uid, groupname);
}

Eina_Bool addToUserGroupByNameAndUsername(DB *db, Zentific_Ctx *ctx, const char *username, const char *groupname){
	QE(
		"INSERT INTO `membership` (`uid`,`gid`) VALUES "
		"("
			"(SELECT uid FROM `users` WHERE username = '%s' LIMIT 1), "
			"(SELECT gid FROM `groups` WHERE name = '%s' LIMIT 1)"
		")",
		username, groupname);
}

Eina_Bool removeFromUserGroupByNameAndUsername(DB *db, Zentific_Ctx *ctx, const char *username, const char *groupname){
	QE(
		"DELETE FROM `membership` "
		"WHERE `uid` = "
			"(SELECT uid FROM `users` WHERE username = '%s' LIMIT 1) "
		"AND `gid` = "
			"(SELECT gid FROM `groups` WHERE name = '%s' LIMIT 1) ",
		username, groupname);
}

Eina_Bool isUserGroupMemberByNameAndUsername(DB *db, Zentific_Ctx *ctx, const char *username, const char *groupname){
	QIAB(
		"SELECT `uid` FROM `membership` "
		"WHERE `uid` = "
			"(SELECT uid FROM `users` WHERE username = '%s' LIMIT 1) "
		"AND `gid` = "
			"(SELECT gid FROM `groups` WHERE name = '%s' LIMIT 1) ",
		username, groupname);
}
 /*********************************************************************************
 * GROUP FUNCTIONS MISC
 */
Eina_Bool addUserGroup(DB *db, Zentific_Ctx *ctx, USER_GROUP *ug){
	QEJ(
		"INSERT INTO `groups`"
		" (`privilege`,`name`,`desc`)"
		" VALUES ('%d','%s','%s')",
		ug->privilege, ug->name, ug->desc);
}

Eina_Bool getAllUserGroups(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	if ((offset < 0) || (recordcount < 0))
		QIS(
			"SELECT `gid` FROM `groups`");
	QISA(
		"SELECT `gid` FROM `groups` LIMIT %d, %d",
		offset, recordcount);
}

Eina_Bool getAllUserGroupNames(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	if ((offset < 0) || (recordcount < 0))
		QSS(
			"SELECT `name` FROM `groups`");
	QSSA(
		"SELECT `name` FROM `groups` LIMIT %d, %d",
		offset, recordcount);
}

Eina_Bool getAllUserGroupsFull(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	DB_VARS;
	if ((offset < 0) || (recordcount < 0))
		QUERY_NOESCAPE(Array_USER_GROUP_esql,
			"SELECT * FROM `groups`");
	else
		QUERY(Array_USER_GROUP_esql,
			"SELECT * FROM `groups` LIMIT %d, %d",
			offset, recordcount);
	QID_RETURN;
}

Eina_Bool getUserGroupsByUID(DB *db, Zentific_Ctx *ctx, int uid){
	QISA(
		"SELECT `gid` FROM `membership` WHERE `uid`='%d'",
		uid);
}

Eina_Bool getUserGroupsByName(DB *db, Zentific_Ctx *ctx, const char *username){
	QISA(
		"SELECT gid FROM membership "
		"JOIN users ON membership.uid=users.uid "
		"WHERE username='%s' LIMIT 1",
		username);
}

Eina_Bool getUserGroupNamesByUID(DB *db, Zentific_Ctx *ctx, int uid){
	QSSA(
		"SELECT name FROM membership "
		"JOIN users ON membership.uid=users.uid "
		"WHERE uid='%i' LIMIT 1",
		uid);
}

Eina_Bool getUserGroupNamesByName(DB *db, Zentific_Ctx *ctx, const char *username){
	QSA(
		"SELECT name FROM groups "
		"JOIN membership ON groups.gid=membership.gid "
		"JOIN users ON membership.uid=users.uid "
		"WHERE username='%s' LIMIT 1",
		username);
}

Eina_Bool getAllUserGroupsSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QISA(
		"SELECT id FROM `groups` "
		"ORDER BY `%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);
}

Eina_Bool getAllUserGroupsFullSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	DB_VARS;
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QUERY(Array_USER_GROUP_esql,
		"SELECT * FROM `groups` "
		"ORDER BY `%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);
	ctx->free_func = (Ecore_Cb)Array_USER_GROUP_free;
	QID_RETURN;
}

Eina_Bool getUserGroupCount(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT COUNT(gid) FROM `groups`");
}
