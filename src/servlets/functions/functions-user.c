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
#include "functions-session.h"
#include "functions-auth.h"
#include "functions-console.h"
#include "zentific-defines.h"


static void
user_cb_name(Zentific_Ctx *ctx, Eina_Bool ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	if (!ret){
		ctx->error = 0;
		ctx->count++;
		REWIND;
		RESUME;
		return;
	}
	ctx->error = AZY_ERR(ENTITY_ALREADY_EXISTS);
	RESUME;
}

static void
user_cb_lang(Zentific_Ctx *ctx, Eina_Bool ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	if (!ret){
		ctx->error = 0;
		ctx->count++;
		REWIND;
		RESUME;
		return;
	}
	ctx->error = AZY_ERR(INVALID_PARAMETER);
	RESUME;
}

static Eina_Bool
user_setattr_check(Zentific_Ctx *ctx, const char *attribute, const char *value){
	if (!zshare_strcmp(attribute, "name")){
		if (eina_strlen_bounded(value, MAX_USERNAME_LENGTH + 1) > MAX_USERNAME_LENGTH){
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			CTX_RETURN;
		}
		if (!ctx->count){
			db->isUserName(db, ctx, value);
			CTX_CB_ADD(user_cb_name);
			CTX_RETURN;
		}
	} else if (!zshare_strcmp(attribute, "email")){
		char *at, *dot;
		//FIXME: add email validation function or is this lazy attempt enough?
		if (!(at = strrchr(value, '@')) || !(dot = strrchr(value, '.')) ||
			(dot - at < 0)){
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			CTX_RETURN;
		}
	} else if (!zshare_strcmp(attribute, "active")){
		if (zshare_strcmp(value, "0") || zshare_strcmp(value, "1")){
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			CTX_RETURN;
		}
	} else if (!zshare_strcmp(attribute, "type")){
		int t;

		if (value[1]){
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			CTX_RETURN;
		}

		errno = 0;
		t = strtol(value, NULL, 10);
		if (errno){
			WRN("Failed to convert '%s' to int because of error: %s", value, strerror(errno));
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			CTX_RETURN;
		}

		if ((t < USER_TYPE_SUPER_USER) || (t > USER_TYPE_NODE)){
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			CTX_RETURN;
		}
	} else if (!zshare_strcmp(attribute, "language")){
		if (ctx->count < 2){
			db->getLanguageCode(db, ctx, strtol(value, NULL, 10));
			CTX_CB_ADD(user_cb_lang);
			CTX_RETURN;
		}
	} else {
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	CTX_RETURN;
}


static void
user_cb_logout_deauth(Zentific_Ctx *ctx, Eina_Bool ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	if (!ret)
		/* FIXME */
		ERR("Session could not be deauthenticated");
	if (--ctx->count) return;
	if (!ctx->cbs) AZY_RETVAL(!azy_content_error_is_set(ERROR));
	NEXT_CB();
}

static void
user_cb_logoutsessions(Zentific_Ctx *ctx, Eina_List *ret){
	const char *session_id;

	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	if (!ret) RESUME; /* error already set */
	ctx->count = eina_list_count(ret);
	EINA_LIST_FREE(ret, session_id){
		_deauthSession(ctx, session_id);
		eina_stringshare_del(session_id);
		CTX_CB_ADD(user_cb_logout_deauth);
	}
}

static void
user_cb_superuser(Zentific_Ctx *ctx, int ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	if (!ctx->cbs) AZY_RET(ret == USER_TYPE_SUPER_USER);
	NEXT_CB(ret == USER_TYPE_SUPER_USER);
}

static void
user_cb_generatepw(Zentific_Ctx *ctx, Eina_Bool ret){
	const char *p;
	MARK_CB;
	DBG("(ctx=%p, ret=%c)", ctx, ret);
	p = CTX_PARAM_GET;
	if (!ctx->cbs) AZY_RETVAL(p);
	NEXT_CB(p);
}

/***********************************************************
 * USER FUNCTIONS BY UID
 */
Eina_Bool _isUserUID(Zentific_Ctx *ctx, int uid){
	if (uid < 0){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}

	db->isUserUID(db, ctx, uid);
	CTX_RETURN;
}

Eina_Bool _isActive(Zentific_Ctx *ctx, int uid){
	db->isActive(db, ctx, uid);
	CTX_RETURN;
}

Eina_Bool _modifyUser(Zentific_Ctx *ctx, int uid, USER *u){
	if ((!u->username[0]) || (eina_strlen_bounded(u->username, MAX_USERNAME_LENGTH + 1) > MAX_USERNAME_LENGTH)){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	db->modifyUser(db, ctx, uid, u);
	CTX_RETURN;
}


Eina_Bool _getUser(Zentific_Ctx *ctx, int uid){
	db->getUser(db, ctx, uid);
	CTX_RETURN;
}

Eina_Bool _getUserType(Zentific_Ctx *ctx, int uid){
	db->getUserType(db, ctx, uid);
	CTX_RETURN;
}

Eina_Bool _isSuperUser(Zentific_Ctx *ctx, int uid){
	_getUserType(ctx, uid);
	CTX_CB_ADD(user_cb_superuser);
	CTX_RETURN;
}

Eina_Bool _removeUser(Zentific_Ctx *ctx, int uid){
	db->removeUser(db, ctx, uid);
	CTX_RETURN;
}

/* TODO make salt length configurable */
/* TODO password complexity rules ?? */
Eina_Bool _setPassword(Zentific_Ctx *ctx, int uid, const char *pass){
	int saltlen 	= DEFAULT_SALT_LENGTH;
	char *salt	= NULL;
	char *hash	= NULL;

	//NOTE: we  dont care if user isActive();
	//	user could have (e.g.) exceeded
	//	login failures, account locked (inactive)
	//	and setting password (resetting) might be
	//	part of re-enabling

	if(eina_strlen_bounded(pass, MIN_PASSWD_LEN) < MIN_PASSWD_LEN){
		if (!azy_content_error_is_set(ERROR))
			azy_content_error_faultmsg_set(ERROR, AZY_ERR(USER_PASSWORD),
				"Password must be at least "S(MIN_PASSWD_LEN)" characters long.");
		CTX_RETURN;
	}

	//FIXME vetting of this algo, here and in auth_db
	if (!(salt = generate_random_salt(saltlen)))
		CTX_RETURN;

	if (!(hash = sha1_buffer(salt, pass))) {
		free(salt);
		CTX_RETURN;
	}

	db->setPassword(db, ctx, uid, hash, salt);

	free(salt);
	free(hash);

	CTX_RETURN;
}

Eina_Bool _getUsername(Zentific_Ctx *ctx, int uid){
	db->getUsername(db, ctx, uid);
	CTX_RETURN;
}

/* generates a password and sets it accordingly, returns pass.
 *	TODO: should pwlen set here be configurable or a define?
 */
Eina_Bool _generatePassword(Zentific_Ctx *ctx, int uid){
	int pwlen = MIN_PASSWD_LEN+2;
	char *ret;

	if (!(ret = generate_passwd(pwlen))){
		ctx->error = AZY_ERR(SERVER_FAILURE);
		CTX_RETURN;
	}

	_setPassword(ctx, uid, ret);
	CTX_CB_ADD(user_cb_generatepw);
	CTX_PARAM_ADD(user_cb_generatepw, eina_stringshare_add(ret));
	free(ret);
	CTX_RETURN;
}

Eina_Bool _setUserAttribute(Zentific_Ctx *ctx, int uid, const char *attribute, const char *value){
	if (user_setattr_check(ctx, attribute, value))
		db->setUserAttribute(db, ctx, uid, attribute, value);
	CTX_RETURN;
}

Eina_Bool _logoutByUID(Zentific_Ctx *ctx, int uid){
	db->getSessionsByUID(db, ctx, uid);
	CTX_CB_ADD(user_cb_logoutsessions);
	CTX_RETURN;
}

/*******************************************************
 * USER FUNCTIONS BY NAME
 */
Eina_Bool _isUserName(Zentific_Ctx *ctx, const char *name){
	if (eina_strlen_bounded(name, MAX_USERNAME_LENGTH + 1) > MAX_USERNAME_LENGTH){
		if (!azy_content_error_is_set(ERROR))
			ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	db->isUserName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _isActiveByName(Zentific_Ctx *ctx, const char *name){
	db->isActiveByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _modifyUserByName(Zentific_Ctx *ctx, const char *name, USER *u){
	if (eina_strlen_bounded(u->username, MAX_USERNAME_LENGTH + 1) > MAX_USERNAME_LENGTH || !u->username[0]){
		if (!azy_content_error_is_set(ERROR))
			ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	db->modifyUserByName(db, ctx, name, u);
	CTX_RETURN;
}

Eina_Bool _removeUserByName(Zentific_Ctx *ctx, const char *name){
	if (eina_strlen_bounded(name, MAX_USERNAME_LENGTH + 1) > MAX_USERNAME_LENGTH || !name[0]){
		if (!azy_content_error_is_set(ERROR))
			ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	db->removeUserByName(db, ctx, name);
	CTX_RETURN;
}

/* TODO password complexity rules ?? */
Eina_Bool _setPasswordByName(Zentific_Ctx *ctx, const char *name, const char *pass){
	int saltlen 	= DEFAULT_SALT_LENGTH;
	char *salt 	= NULL;
	char *hash 	= NULL;
	//NOTE: we  dont care if user isActive();
	//	user could have (e.g.) exceeded
	//	login failures, account locked (inactive)
	//	and setting password (resetting) might be
	//	part of re-enabling
	if (eina_strlen_bounded(name, MAX_USERNAME_LENGTH + 1) > MAX_USERNAME_LENGTH || !name[0]){
		if (!azy_content_error_is_set(ERROR))
			ctx->error = AZY_ERR(INVALID_PARAMETER);
		CTX_RETURN;
	}
	if(eina_strlen_bounded(pass, MIN_PASSWD_LEN) < MIN_PASSWD_LEN){
		if (!azy_content_error_is_set(ERROR))
			azy_content_error_faultmsg_set(ERROR,
				AZY_ERR(USER_PASSWORD),
				"Password must be at least "S(MIN_PASSWD_LEN)" characters long.");
		CTX_RETURN;
	}

	//FIXME vetting of this algo, here and in auth_db
	if(!(salt = generate_random_salt(saltlen))){
		ERR("Memory allocation failed");
		CTX_RETURN;
	}

	if (!(hash = sha1_buffer(salt, pass))) {
		free(salt);
		CTX_RETURN;
	}

	db->setPasswordByName(db, ctx, name, hash, salt);

	free(salt);
	free(hash);

	CTX_RETURN;
}


/* generates a password and sets it accordingly, returns pass
 *	TODO merge this needlessly duplicated function with
 *		generatePasswordByName
 *	TODO: should pwlen set here be configurable or a define?
 */
Eina_Bool _generatePasswordByName(Zentific_Ctx *ctx, const char *name){
	int pwlen = MIN_PASSWD_LEN+2;
	char *ret;

	if (!(ret = generate_passwd(pwlen))){
		ctx->error = AZY_ERR(SERVER_FAILURE);
		CTX_RETURN;
	}

	_setPasswordByName(ctx, name, ret);
	CTX_CB_ADD(user_cb_generatepw);
	CTX_PARAM_ADD(user_cb_generatepw, eina_stringshare_add(ret));
	free(ret);
	CTX_RETURN;
}

Eina_Bool _setUserAttributeByName(Zentific_Ctx *ctx, const char *name, const char *attribute, const char *value){
	if (user_setattr_check(ctx, attribute, value))
		db->setUserAttributeByName(db, ctx, name, attribute, value);
	CTX_RETURN;
}

Eina_Bool _logoutByName(Zentific_Ctx *ctx, const char *name){
	db->getSessionsByName(db, ctx, name);
	CTX_CB_ADD(user_cb_logoutsessions);
	CTX_RETURN;
}

Eina_Bool _getUserByName(Zentific_Ctx *ctx, const char *name){
	db->getUserByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _getUserTypeByName(Zentific_Ctx *ctx, const char *name){
	db->getUserTypeByName(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _isSuperUserByName(Zentific_Ctx *ctx, const char *name){
	_getUserTypeByName(ctx, name);
	CTX_CB_ADD(user_cb_superuser);
	CTX_RETURN;
}

Eina_Bool _getActiveUserUID(Zentific_Ctx *ctx, const char *name){
	db->getActiveUserUID(db, ctx, name);
	CTX_RETURN;
}

Eina_Bool _getUserUID(Zentific_Ctx *ctx, const char *name){
	db->getUserUID(db, ctx, name);
	CTX_RETURN;
}

/*******************************************************
 * USER FUNCTIONS MISC
 */

//FIXME this should probably be elsewhere; its not really exposed into the interface atm
Eina_Bool _getLanguageCode(Zentific_Ctx *ctx, int id){
	db->getLanguageCode(db, ctx, id);
	CTX_RETURN;
}

//NOTE to avoid setting type implicitly to superuser or
//	similar:
//	a) set default value for type in db (same for active)
//	b) do checking on the frontend
//	c) and maybe, do checking here(?)
Eina_Bool _addUser(Zentific_Ctx *ctx, USER *user){
	const char *at;
	const char *dot;


	if ((!user->username[0]) || (eina_strlen_bounded(user->username, MAX_USERNAME_LENGTH + 1) > MAX_USERNAME_LENGTH))
		goto error;

	//FIXME: add email validation function. this is woefully inadequate
	if (!(at = strrchr(user->email, '@')) || !(dot = strrchr(user->email, '.')) ||
		(dot - at < 0))
		goto error;

	/* FIXME: make user->active a boolean, but be sure to update all interface and API code that uses it */
	if ((user->active != 0) && (user->active != 1))
		goto error;

	/* USER_TYPE_SUPER_USER == 0, USER_TYPE_NONE == 5 see ztypes.h for the enum */
	if ((user->type < USER_TYPE_SUPER_USER) || (user->type > USER_TYPE_NODE))
		goto error;

	if (!ctx->count){
		db->isUserName(db, ctx, user->username);
		CTX_CB_ADD(user_cb_name);
		CTX_RETURN;
	}

	if (ctx->count < 2){
		db->getLanguageCode(db, ctx, strtol(user->language, NULL, 10));
		CTX_CB_ADD(user_cb_lang);
		CTX_RETURN;
	}

	db->addUser(db, ctx, user);
	CTX_RETURN;
error:
	ctx->error = AZY_ERR(INVALID_PARAMETER);
	CTX_RETURN;
}

Eina_Bool _logout(Zentific_Ctx *ctx, const char *session_id){
	if (!ctx->count){
		_disconnectConsolesBySession(ctx, session_id);
		CTX_CB_ADD(cb_bool_rewind);
	} else
		_deauthSession(ctx, session_id);
	CTX_RETURN;
}

//FIXME might want a more general
//	_getUsers function with flags for
//	'getallinactive' 'getallactive'
//	etc
Eina_Bool _getAllUsers(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield && sortfield[0])
		db->getAllUsersSorted(db, ctx, sortfield, desc, offset, recordcount);
	else
		db->getAllUsers(db, ctx, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getAllUsersFull(Zentific_Ctx *ctx, const char *sortfield, Eina_Bool desc, int offset, int recordcount){
	if (sortfield && sortfield[0])
		db->getAllUsersSorted(db, ctx, sortfield, desc, offset, recordcount);
	else
		db->getAllUsersFull(db, ctx, offset, recordcount);
	CTX_RETURN;
}

Eina_Bool _getLoggedInUsers(Zentific_Ctx *ctx){
	db->getLoggedInUsers(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getLoggedInUsernames(Zentific_Ctx *ctx){
	db->getLoggedInUsernames(db, ctx);
	CTX_RETURN;
}

Eina_Bool _getUserCount(Zentific_Ctx *ctx){
	db->getUserCount(db, ctx);
	CTX_RETURN;
}
