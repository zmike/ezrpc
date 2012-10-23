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
#include <fcntl.h>
#include "functions-auth.h"
#include "functions-user.h"
#include "functions-zentific.h"
#include "zentific-defines.h"
#include <security/pam_appl.h>
#include <security/pam_misc.h>

static int zentific_conv (int num_msg, const struct pam_message **msgm, struct pam_response **response, void *appdata_ptr);

static char *generate_poor_salt(int length);
static Eina_Bool _auth_pam(const char *user, const char *pass);

static char passwdChars[] =
 "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,~:!@#$%&()_+-=[]{};?.";
 //"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz,~`!@#$%&()_+-=[]{};'\"/?\\|.";

static char *generate_poor_salt(int length) {
	char *ret;
	int i;
	srandom(time(NULL));

	if (!(ret = calloc(length + 1, sizeof(char))))
		return NULL;

	for(i = 0; i < length; i++) {
		ret[i] = passwdChars[random() % (sizeof(passwdChars) - 1)];
	}

	return ret;
}

static int
zentific_conv(int num_msg, const struct pam_message **msgm, struct pam_response **response, void *appdata_ptr) {
		const char *pwd = appdata_ptr;
		int n;
		struct pam_response *reply;
		reply = calloc(num_msg, sizeof(struct pam_response));
		if (reply == NULL) {
			fprintf(stderr, "PAM: No memory\n");
			return PAM_CONV_ERR;
		}

		for (n = 0; n < num_msg; n++) {
			switch (msgm[n]->msg_style) {
			case PAM_PROMPT_ECHO_OFF:
				reply[n].resp = strdup(pwd);
				break;
			case PAM_PROMPT_ECHO_ON:
				break;
  			case PAM_ERROR_MSG:
		     		fprintf(stderr, "PAM: Error = %s\n", msgm[n]->msg);
		     		break;
			default:
		     		break;

			}
		}

		*response = reply;
		return PAM_SUCCESS;

}

/* FIXME: async */
static Eina_Bool
_auth_pam(const char *user, const char *pass){
	pam_handle_t *pamh;
	int authenticated = 0;

	DBG("Authenticating user \"%s\" via internal provider.", user);
	struct pam_conv conv = {
		zentific_conv,
		(void *)pass
	};

	int ret;
	ret = pam_start("zrpc", user, &conv, &pamh);

	//set the password token in case a module needs it
	// 	dont check return, as some auth mechanisms may not
	//	support PAM_AUTHTOK
	pam_set_item(pamh, PAM_AUTHTOK, pass);

	// is user really user?
    	if (ret == PAM_SUCCESS){
		ret = pam_authenticate(pamh, PAM_DISALLOW_NULL_AUTHTOK | PAM_SILENT);
    	} else {
		ERR("PAM: %s - %s", user, pam_strerror(pamh, ret));
	}


	// permitted access?
    	if (ret == PAM_SUCCESS){
		ret = pam_acct_mgmt(pamh, PAM_DISALLOW_NULL_AUTHTOK | PAM_SILENT);
	} else {
		ERR("PAM: %s - %s", user, pam_strerror(pamh, ret));
	}

    	// This is where we have been authorized or not.
    	if (ret == PAM_SUCCESS) {
		ERR("PAM: %s - %s", user, pam_strerror(pamh, ret));
		authenticated = 1;
    	} else if (ret == PAM_CRED_ERR ){
		//FIXME verify that this portion of the conditional is required.
		//	it is here on the assumption that pam_groups is being used
		//	to check user's GID/name. semantics may not actually cause
		//	this to ever be triggered
		ERR("PAM: %s group membership error - %s", user, pam_strerror(pamh, ret));
		authenticated = 0;
    	} else {
		ERR("PAM: %s - %s", user, pam_strerror(pamh, ret));
		authenticated = 0;
    	}

	// close Linux-PAM
    	int end = pam_end(pamh,ret);
    	if (end != PAM_SUCCESS) {
		ERR("PAM: %s - %s", user, pam_strerror(pamh, ret));
		pamh = NULL;

		// FIXME dont want exit..find another way
		//exit(1);
    	}

	return authenticated;

}

static Eina_Bool
_auth_db(Zentific_Ctx *ctx, const char *pass){
	Eina_List *auth;
	const char *salt, *hash;
	char *computedhash;
	SESSION *s = SES;
	switch (ctx->count){
		case 0:
			ctx->count++;
		case 1:
			db->getAuth(db, ctx, SES->uid);
			CTX_VALUE_RETURN;
			break;
		case 2:
			auth = CTX_PARAM_GET;
			if ((!auth) || (eina_list_count(auth) != 2)){
				ERR("could not fetch stored data for auth");
				DBG("DB authentication failed, trying PAM...");
				ctx->count = 10;
				ctx->error = 0;
				EINA_LIST_FREE(auth, salt)
					eina_stringshare_del(salt);
				_getConfig(ctx, "authfallback");
				CTX_VALUE_SEND(_auth);
				break;
			}
			salt = auth->data;
			hash = auth->next->data;

			computedhash = sha1_buffer(salt, pass);
			ctx->auth = !zshare_strcmp(computedhash, hash);
			free(computedhash);
			EINA_LIST_FREE(auth, salt)
				eina_stringshare_del(salt);
			if (ctx->auth){
				if (!s->authenticated)
					/* must be login attempt */
					s->authenticated = EINA_TRUE;
				else ctx->count = 0;
			} else
				ctx->error = AZY_ERR(NOT_AUTHORIZED);
			ctx->count++;
			break;
		default:
			break;
	}
	CTX_RETURN;
}

char *generate_random_salt(int length) {
	char *buf	= NULL;
	char *ret	= NULL;
	int fd		= 0;
	int i		= 0;

	if (length >= 256 || length < 1)
		return NULL;

	if ((fd = open("/dev/random", O_RDONLY)) < 0) {
		return generate_poor_salt(length);
	}

	buf = alloca(length);

	if (read(fd, buf, length) != length)
		goto error;

	if (!(ret = calloc(length + 1, sizeof(char))))
		goto error;

	for(i = 0; i < length; i++) {
		ret[i] = passwdChars[abs(buf[i]) % (sizeof(passwdChars) - 1)];
	}

	close(fd);

	return ret;

error:
	close(fd);
	return generate_poor_salt(length);
}

/* TODO make minimum password length configurable */
char *generate_passwd(int length) {

	if (length >= 256 || length < MIN_PASSWD_LEN){
		ERR("invalid length");
		return NULL;
	}

	return generate_random_salt(length);
}

Eina_Bool _auth(Zentific_Ctx *ctx, const char *pass){
	const char *authtype;
	SESSION *s = SES;
	extern const char *true;
	switch (ctx->count){
		case 0:
			_getConfig(ctx, "authtype");
			CTX_VALUE_RETURN;
			break;
		case 1:
			authtype = CTX_PARAM_GET;
			if ((ctx->error == AZY_ERR(UNKNOWN_RESOURCE)) || (!zshare_strcmp(authtype, "internal"))){
				DBG("Authenticating user %d via internal provider.", s->uid);
				ctx->error = 0;
				_auth_db(ctx, pass);
			} else if (!zshare_strcmp(authtype, "pam")) {
				DBG("DB authentication failed, trying PAM...");
				s->authenticated = _auth_pam(s->username, pass);
			} else if (ctx->error) break;
			else {
				_getConfig(ctx, "authfallback");
				CTX_VALUE_RETURN;
				ctx->count = 10;
			}
			eina_stringshare_del(authtype);
			break;
		case 10: /* fallback */
			authtype = CTX_PARAM_GET;
			if (authtype == true)
				ctx->auth = _auth_pam(s->username, pass);
			if (!s->authenticated)
				s->authenticated = ctx->auth;
			else ctx->count = 0;
			if (!ctx->auth) ctx->error = AZY_ERR(NOT_AUTHORIZED);
			break;
		default:
			_auth_db(ctx, pass);
	}
	CTX_RETURN;
}
