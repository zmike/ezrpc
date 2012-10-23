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
/* API Functions: Sessions			   */
/* ----------------------------------------------- */
Eina_Bool getLoggedInUsers(DB *db, Zentific_Ctx *ctx){
	QIS(
		"SELECT `uid` FROM `sessions` WHERE"
		" (UNIX_TIMESTAMP(NOW())-timestamp) <= IFNULL("
		"  (SELECT `value` FROM `config` WHERE `key`='session_timeout')"
		" ,5)*60 AND `authenticated`='1'");
}

Eina_Bool getLoggedInUsernames(DB *db, Zentific_Ctx *ctx){
	QSS(
		"SELECT username FROM `sessions` "
		"JOIN users ON sessions.uid=users.uid "
		"WHERE (UNIX_TIMESTAMP(NOW())-timestamp) <= IFNULL("
		"  (SELECT `value` FROM `config` WHERE `key`='session_timeout')"
		" ,5)*60 AND `authenticated`='1'");
}

Eina_Bool getSessions(DB *db, Zentific_Ctx *ctx){
	QSS(
		"SELECT session_id FROM `sessions`");
}

Eina_Bool getSession(DB *db, Zentific_Ctx *ctx, const char *sessionid){
	DB_VARS;

	QUERY(SESSION_esql,
		"SELECT session_id, authenticated, uid, host, timestamp, referrer, failures FROM `sessions` WHERE session_id='%s'",
		sessionid);
	ctx->free_func = (Ecore_Cb)SESSION_free;
	QID_RETURN;
}

Eina_Bool setSession(DB *db, Zentific_Ctx *ctx, SESSION *session){
	QE(
		"INSERT INTO `sessions`"
		"  (session_id, authenticated, uid, host, timestamp, referrer, failures)"
		" VALUES"
		"  ('%s', '%d', '%d', '%s', '%d', '%s', '%d')"
		" ON DUPLICATE KEY UPDATE"
		"  session_id='%s', authenticated='%d', uid='%d', host='%s', timestamp='%d',"
		"  referrer='%s', failures='%d'",
		session->session_id, session->authenticated, session->uid,
		session->client_host, session->timestamp, session->referrer,
		session->failures,
		session->session_id, session->authenticated, session->uid,
		session->client_host, session->timestamp, session->referrer,
		session->failures);
}

Eina_Bool setLastLogin(DB *db, Zentific_Ctx *ctx, int uid){
	QE(
		"UPDATE `users` SET `lastlogin`=NOW() WHERE `uid`='%d'",
		uid);
}

Eina_Bool deauthSession(DB *db, Zentific_Ctx *ctx, const char *session_id){
	QE(
		"UPDATE `sessions` SET authenticated = '0' WHERE session_id = '%s'",
		session_id);
}

Eina_Bool removeSession(DB *db, Zentific_Ctx *ctx, const char *session_id){
	QE(
		"DELETE FROM `sessions` where session_id = '%s'",
		session_id);
}

Eina_Bool getUserBySession(DB *db, Zentific_Ctx *ctx, const char *session_id){
	QIA(
		"SELECT `uid` FROM `sessions` WHERE `session_id`='%s'",
		session_id);

}

Eina_Bool getUserNameBySession(DB *db, Zentific_Ctx *ctx, const char *session_id){
	QSA(
		"SELECT username FROM sessions "
		"JOIN users ON sessions.uid=users.uid "
		"WHERE session_id='%s'",
		session_id);
}
