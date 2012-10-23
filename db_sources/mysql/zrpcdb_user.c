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



/* ----------------------------------------------- */
/* API Functions: Users				   */
/* ----------------------------------------------- */
/*******************************************************
 * USER FUNCTIONS BY UID
 */

Eina_Bool getUser(DB *db, Zentific_Ctx *ctx, int uid){
	DB_VARS;
	QUERY(USER_esql,
		"SELECT * FROM `users` LEFT JOIN languages ON users.lang=languages.id WHERE uid='%d'",
		uid);
	ctx->free_func = (Ecore_Cb)USER_free;
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

Eina_Bool isUserUID(DB *db, Zentific_Ctx *ctx, int uid){
	DB_VARS;
	ctx->state = 1;
	QUERY(esql_res_to_lli,
		"SELECT uid FROM `users` WHERE uid='%d'", uid);
	QID_CB_ADD(zrpcdb_cb_int);
	QID_RETURN;
}

Eina_Bool setUserType(DB *db, Zentific_Ctx *ctx, int uid, int type){
	DB_VARS;
	QUERY_NOCONVERT(
		"UPDATE `users` SET type='%d' WHERE uid='%d'",
		type, uid);
	QID_CB_ADD(zrpcdb_cb_exec);
	QID_RETURN;
}

Eina_Bool getUserType(DB *db, Zentific_Ctx *ctx, int uid){
	QIA(
		"SELECT type FROM `users` WHERE uid = '%d'", uid);
}

Eina_Bool getUsername(DB *db, Zentific_Ctx *ctx, int uid){
	QSA(
		"SELECT username FROM `users` WHERE uid='%d'", uid);
}

Eina_Bool getSessionsByUID(DB *db, Zentific_Ctx *ctx, int uid){
	QISA(
		"SELECT session_id FROM `sessions` WHERE uid='%d'",
		uid);
}

Eina_Bool isVMowner(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid){
	//FIXME make sure privilege level here will exclude users with NO privilege. therefore an
	//	entry in the ownership table is but priv is set in such a way to EXCLUDE ownership
	DB_VARS;
	ctx->state = 1;
	ctx->free_func = NULL;
	QUERY(DUMMY_esql,
		"SELECT vms.uuid, ownershipvm.privileges FROM ownershipvm, vms"
			" WHERE vms.id=ownershipvm.vm"
			" AND ownershipvm.privileges != 4" //4==NONE privilege aka excluded
			" AND ownershipvm.uid='%d' AND vms.uuid='%s'",
			uid, uuid);
	QID_CB_ADD(zrpcdb_cb_bool);
	QID_RETURN;
}

Eina_Bool isNODEowner(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid){
	DB_VARS;
	ctx->state = 1;
	ctx->free_func = NULL;
	QUERY(DUMMY_esql,
		"SELECT nodes.uuid, ownershipnode.privileges FROM ownershipnode, nodes"
			" WHERE nodes.id=ownershipnode.node"
			" AND ownershipnode.privileges != 4" //4==NONE privilege aka excluded
			" AND ownershipnode.uid='%d' AND nodes.uuid='%s'",
			uid, uuid);
	QID_CB_ADD(zrpcdb_cb_bool);
	QID_RETURN;
}


Eina_Bool setUserAttribute(DB *db, Zentific_Ctx *ctx, int uid, const char *attribute, const char *value){
	QE(
		"UPDATE `users` SET `%s` = '%s' WHERE uid='%d'",
		attribute, value, uid);
}

Eina_Bool isActive(DB *db, Zentific_Ctx *ctx, int uid){
	QBA(
		"SELECT active FROM `users` WHERE uid='%d'", uid);
}

Eina_Bool removeUser(DB *db, Zentific_Ctx *ctx, int uid) {
	QE(
		"DELETE FROM `users` WHERE uid='%d'", uid);
}

Eina_Bool setPassword(DB *db, Zentific_Ctx *ctx, int uid, const char *hash, const char *salt){
	QE(
		"UPDATE `users` SET `salt` = '%s', `hash` = '%s'"
		" WHERE `uid` = '%d'",
		salt, hash, uid);
}

Eina_Bool getPrivilege(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int node_vm){
	char *type = NULL;

	if (!node_vm)
		type = "node";
	else if (node_vm == 1)
		type = "vm";
	else {
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}
	QIA(
		"SELECT `privileges` FROM `ownership%s` "
		"LEFT JOIN %ss ON ownership%s.%s=%ss.id "
		"WHERE %ss.uuid='%s' "
		"AND `uid`='%d' "
		"AND `gid`='%d'",
		type, type, type, type, type, type, uuid, uid, gid);
}

Eina_Bool grantPrivilege(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int level, int node_vm){
	char *type = NULL;

	if (!node_vm)
		type = "node";
	else if (node_vm == 1)
		type = "vm";
	else {
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}
	QE(
		"INSERT INTO `ownership%s` ( %s, uid, gid, privileges)"
		" VALUES ("
		"	(SELECT `id` FROM `%ss` WHERE uuid='%s'), '%d', '%d', '%d')",
		type, type, type, uuid, uid, gid, level);
}

Eina_Bool revokePrivilege(DB *db, Zentific_Ctx *ctx, const char *uuid, int uid, int gid, int node_vm){
	char *type = NULL;

	if (!node_vm)
		type = "node";
	else if (node_vm == 1)
		type = "vm";
	else {
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}
	QE(
		"USING ownership%s "
		"LEFT JOIN %ss ON ownership%s.%s=%ss.id "
		"WHERE %ss.uuid='%s' "
		"AND uid='%d' "
		"AND gid='%d'",
		type, type, type, type, type, type, uuid, uid, gid);
}

Eina_Bool modifyUser(DB *db, Zentific_Ctx *ctx, int uid, USER *u){
	QE(
		"UPDATE `users` "
		"SET `username`='%s', `givenname`='%s', `active`='%d', `type`='%d', `lang`="
			"(SELECT id FROM languages WHERE language='%s'), "
		"`email`='%s' "
		"WHERE `uid`='%d' "
		"AND languages.language='%s'",
		u->username, u->givenname, u->active, u->type, u->language, u->email, uid);
}

/*******************************************************
 * USER FUNCTIONS BY NAME
 */
Eina_Bool isUserName(DB *db, Zentific_Ctx *ctx, const char *name){
	QBA(
		"SELECT uid FROM `users` WHERE username = '%s'", name);
}

Eina_Bool getUserUID(DB *db, Zentific_Ctx *ctx, const char *name){
	DB_VARS;
	ctx->state = 1;
	QUERY(esql_res_to_lli,
		"SELECT uid FROM `users` WHERE username='%s'", name);
	QID_CB_ADD(zrpcdb_cb_int);
	QID_RETURN;
}

Eina_Bool getActiveUserUID(DB *db, Zentific_Ctx *ctx, const char *name){
	DB_VARS;
	ctx->state = 1;
	QUERY(esql_res_to_lli,
		"SELECT uid FROM `users` WHERE active='1' AND username='%s'", name);
	QID_CB_ADD(zrpcdb_cb_int);
	QID_RETURN;
}

Eina_Bool getUserTypeByName(DB *db, Zentific_Ctx *ctx, const char *name){
	QIA(
		"SELECT type FROM `users` WHERE username = '%s'", name);
}

Eina_Bool getUserByName(DB *db, Zentific_Ctx *ctx, const char *name){
	DB_VARS;
	QUERY(USER_esql,
		"SELECT * FROM `users` WHERE username = '%s'", name);
	ctx->free_func = (Ecore_Cb)USER_free;
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

Eina_Bool getSessionsByName(DB *db, Zentific_Ctx *ctx, const char *name){
	QSSA(
		"SELECT session_id FROM `sessions` "
		"LEFT JOIN users ON sessions.uid=users.uid "
		"WHERE users.username='%s'",
		name);
}

Eina_Bool isVMownerByName(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name){
	//FIXME make sure privilege level here will exclude users with NO privilege. therefore an
	//	entry in the ownership table is but priv is set in such a way to EXCLUDE ownership
	QBA(
		"SELECT uuid, ownershipvm.privileges FROM vms "
		"LEFT JOIN ownershipvm ON vms.id=ownershipvm.vm "
		"LEFT JOIN users ON ownershipvm.uid=users.uid "
		"WHERE users.username='%s' "
		"AND ownershipvm.privileges != 4 " //4==NONE privilege aka excluded
		"AND vms.uuid='%s'",
		name, uuid);
}

Eina_Bool isNODEownerByName(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name){
	QBA(
		"SELECT uuid, ownershipnode.privileges FROM nodes "
		"LEFT JOIN ownershipnode ON nodes.id=ownershipnode.node "
		"LEFT JOIN users ON ownershipnode.uid=users.uid "
		"WHERE users.username='%s' "
		"AND ownershipnode.privileges != 4 " //4==NONE privilege aka excluded
		"AND nodes.uuid='%s'",
		name, uuid);
}


Eina_Bool setUserAttributeByName(DB *db, Zentific_Ctx *ctx, const char *name, const char *attribute, const char *value){
	QE(
		"UPDATE `users` SET `%s` = '%s' WHERE username='%s'",
		attribute, value, name);
}

Eina_Bool isActiveByName(DB *db, Zentific_Ctx *ctx, const char *name){
	QIA(
		"SELECT uid FROM `users` WHERE username='%s'",
		name);
}

Eina_Bool removeUserByName(DB *db, Zentific_Ctx *ctx, const char *name) {
	QE(
		"DELETE FROM `users` WHERE username='%s'",
		name);
}

Eina_Bool setPasswordByName(DB *db, Zentific_Ctx *ctx, const char *name, const char *hash, const char *salt){

	QE(
		"UPDATE `users` SET `salt` = '%s', `hash` = '%s'"
		" WHERE username='%s'",
		salt, hash, name);
}

Eina_Bool getPrivilegeByName(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name, int node_vm){
	char *type = NULL;

	if (!node_vm)
		type = "node";
	else if (node_vm == 1)
		type = "vm";
	else {
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}
	QIA(
		"SELECT `privileges` FROM `ownership%s` "
		"JOIN users ON ownership%s.uid=users.uid "
		"JOIN %ss ON ownership%s.%s=%ss.uuid "
		"WHERE %ss.uuid='%s' "
		"AND users.username='%s'",
		type, type, type, type, type, type, type, uuid, name);
}

Eina_Bool grantPrivilegeByName(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name, int level, int node_vm){
	char *type = NULL;

	if (!node_vm)
		type = "node";
	else if (node_vm == 1)
		type = "vm";
	else {
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}
	QE(
		"INSERT INTO `ownership%s` ( %s, uid, privileges)"
		" VALUES ("
		"	(SELECT `id` FROM `%ss` WHERE uuid='%s'), "
			"(SELECT uid FROM `users` WHERE username='%s') "
		", '%d')",
		type, type, type, uuid, name, level);
}

Eina_Bool revokePrivilegeByName(DB *db, Zentific_Ctx *ctx, const char *uuid, const char *name, int node_vm){
	char *type = NULL;

	if (!node_vm)
		type = "node";
	else if (node_vm == 1)
		type = "vm";
	else {
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}
	QE(
		"DELETE FROM `ownership%s` "
		"JOIN users ON ownership%s.uid=users.uid "
		"JOIN %ss ON ownership%s.%s=%ss.id "
		"WHERE %ss.uuid='%s' "
		"AND users.username='%s'",
		type, type, type, type, type, type, type, uuid, name);
}

Eina_Bool modifyUserByName(DB *db, Zentific_Ctx *ctx, const char *name, USER *u){
	QE(
		"UPDATE `users` "
		"JOIN languages ON users.lang=languages.id "
		"SET `username`='%s', `givenname`='%s', `active`='%d', `type`='%d', `lang`=languages.id, "
		"`email`='%s'"
		"WHERE `name`='%s' "
		"AND languages.name='%s'",
		u->username, u->givenname, u->active, u->type, u->email, name, u->language);
}
 /*******************************************************
 * USER FUNCTIONS MISC
 */
Eina_Bool getLanguages(DB *db, Zentific_Ctx *ctx){
	DB_VARS;
	QUERY_NOESCAPE(Array_string_esql,
		"SELECT `language` FROM `languages`");
	ctx->free_func = (Ecore_Cb)Array_string_free;
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

Eina_Bool getLanguageIds(DB *db, Zentific_Ctx *ctx){
	DB_VARS;
	QUERY_NOESCAPE(Array_int_esql,
		"SELECT `id` FROM `languages`");
	ctx->free_func = (Ecore_Cb)eina_list_free;
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

Eina_Bool getLanguageCode(DB *db, Zentific_Ctx *ctx, int id){
	DB_VARS;
	ctx->state = 1;
	QUERY(esql_res_to_string,
		"SELECT language FROM `languages` WHERE id='%d'", id);
	QID_CB_ADD(zrpcdb_cb_ptr);
	ctx->free_func = (Ecore_Cb)eina_stringshare_del;
	QID_RETURN;
}

Eina_Bool getLanguageId(DB *db, Zentific_Ctx *ctx, const char *lang){
	DB_VARS;
	ctx->state = 1;
	QUERY(esql_res_to_lli,
		"SELECT id FROM `languages` WHERE language='%s'", lang);
	QID_CB_ADD(zrpcdb_cb_int);
	QID_RETURN;
}

Eina_Bool addUser(DB *db, Zentific_Ctx *ctx, USER *user) {
	QE(
		"INSERT INTO `users` ( username, givenname, email, active, type)"
		" VALUES ('%s', '%s', '%s', '%d', '%d')",
		user->username, user->givenname, user->email, user->active, user->type);
}

Eina_Bool getAllUsers(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	DB_VARS;
	if ((offset < 0) || (recordcount < 0))
		QUERY_NOESCAPE(Array_int_esql,
			"SELECT uid FROM `users`");
	else
		QUERY(Array_int_esql,
			"SELECT uid FROM `users` LIMIT %d, %d",
			offset, recordcount);
	ctx->free_func = (Ecore_Cb)eina_list_free;
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

Eina_Bool getAllUsersFull(DB *db, Zentific_Ctx *ctx, int offset, int recordcount){
	DB_VARS;
	if ((offset < 0) || (recordcount < 0))
		QUERY_NOESCAPE(Array_USER_esql,
			"SELECT * FROM `users`");
	else
		QUERY(Array_USER_esql,
			"SELECT * FROM `users` LIMIT %d, %d",
			offset, recordcount);
	ctx->free_func = (Ecore_Cb)Array_USER_free;
	QID_CB_ADD(zrpcdb_cb_ptr);
	QID_RETURN;
}

Eina_Bool getAllUsersSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QISA(
		"SELECT id FROM `users` "
		"ORDER BY `%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);
}

Eina_Bool getAllUsersFullSorted(DB *db, Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	DB_VARS;
	char limit[64] = {0};

	if ((offset >= 0) && (recordcount >= 0))
		snprintf(limit, sizeof(limit), " LIMIT %d, %d", offset, recordcount);

	QUERY(Array_USER_esql,
		"SELECT * FROM `users` "
		"ORDER BY `%s` %s %s",
		sortfield,
		(desc) ? "DESC" : "",
		limit);
	ctx->free_func = (Ecore_Cb)Array_USER_free;
	QID_RETURN;
}

Eina_Bool getUserCount(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT COUNT(uid) FROM `users`");
}
