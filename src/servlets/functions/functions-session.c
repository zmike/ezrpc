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
#include "functions-session.h"
#include "functions-user.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "zentific-defines.h"

static void
session_cb_error_clear(Zentific_Ctx *ctx, void *ret __UNUSED__){
	if (ctx->dead) return;
	MARK_CB;
	DBG("(ctx=%p)", ctx);
	ctx->error = 0;
	azy_server_module_events_resume(ctx->parent, EINA_TRUE);
}

static void
session_getClientIP_cb(Zentific_Ctx *ctx, const char *proxied){
	Azy_Net *net;
	extern const char *true;
	const char *forwarded, *lastcomma, *proxiedip = NULL;

	MARK_CB;

	DBG("(ctx=%p, proxied=%s)", ctx, proxied);
	net = azy_server_module_net_get(ctx->parent);

	if (proxied != true) goto noproxy;

	//this takes a string (intended to be an IP) direct from the header
	//	so there is a danger here for manipulation.
	if (!(forwarded = azy_net_header_get(net, "X-Forwarded-For")))
		goto noproxy;

	if ((lastcomma = strrchr(forwarded, ','))){
		/* X-Forwarded-For, if using chained reverse proxies, will look like
		 *	 X-Forwarded-For: 127.0.0.1, a.b.c.d, 1.2.3.4
		 *	Where the last IP in the list is the actual host of origin.
		 *	This is not in RFC2616 or any other, but is effectiveely a de-facto
		 *	standard (introduced by squid proxy, used by many many others)
		 */

		/* 9 == at least ", X.X.X.X" */
		if (eina_strlen_bounded(lastcomma, 10) >= 9) {
			/* comma and space after */
			lastcomma += 2;

			//some validation, but really this can be spoofed.
			if (zshare_validate_ip(lastcomma))
				proxiedip = lastcomma;
			else
				ERR("chained proxy client parsing failure. value was '%s'",
					lastcomma);
		}
	} else {
		//this should be a naked IP in the X-Forwarded-For header
		/* 9 == at least "X.X.X.X" */
		if (eina_strlen_bounded(forwarded, 8) >= 7) {
			//some validation, though this is entirely client-supplied..
			if (zshare_validate_ip(forwarded))
				proxiedip = forwarded;
			else
				ERR("proxy client parsing failure. value was '%s'",
					forwarded);
		}
	}

noproxy:
	if (proxiedip){
		NEXT_CB(proxiedip);
		return;
	}
	NEXT_CB(azy_net_ip_get(azy_server_module_net_get(ctx->parent)));
}

static void
session_getClientIP(Zentific_Ctx *ctx){
	DBG("(ctx=%p)", ctx);
	_getConfig(ctx, "proxied");
	CTX_CB_ADD(session_getClientIP_cb);
}

static void
session_checker(Zentific_Ctx *ctx, const char *configured_timeout, SESSION *session){
	time_t now, timeout;

	if ((!configured_timeout) || ctx->error){
		timeout = DEFAULT_SESSION_TIMEOUT * 60;
		ctx->error = 0;
	} else {
		int t;
		errno = 0;
		t = strtol(configured_timeout, NULL, 10);
		STRTO_ERRNO_CHECK(configured_timeout);
		if (errno) t = DEFAULT_SESSION_TIMEOUT;
		timeout = t * 60;
	}

	now = time(NULL);
	DBG("checking session_id='%s'.", session->session_id);
	ctx->checked = EINA_TRUE;
	if (!session->authenticated)
		session->status = AZY_ERRCODE(NOT_AUTHORIZED);
	else {
		if (now - session->timestamp <= timeout)
			session->status = AZY_ERRCODE(VALID_SESSION);
		else
			session->status = AZY_ERRCODE(EXPIRED_SESSION);
	}
}

static void
session_checkuuid_cb(Zentific_Ctx *ctx, const char *configured_timeout){
	SESSION *session;
	int status;
	MARK_CB;
	DBG("(ctx=%p, configured_timeout=%s)", ctx, configured_timeout);

	session = CTX_PARAM_GET;
	session_checker(ctx, configured_timeout, session);
	status = session->status;
	SESSION_free(session);
	if (!ctx->cbs) AZY_RETVAL(status);
	NEXT_CB(status);
}

static void
session_checkuuid_get_cb(Zentific_Ctx *ctx, SESSION *ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);

	_getConfig(ctx, "session_timeout");
	CTX_CB_ADD(session_checkuuid_cb);
	CTX_PARAM_ADD(session_checkuuid_cb, ret);
}

Eina_Bool _checkSessionUUID(Zentific_Ctx *ctx, const char *uuid){
	if (!zshare_validate_uuid(uuid)){
		ctx->error = AZY_ERR(INVALID_SESSION);
		CTX_RETURN;
	}

	_getSession(ctx, uuid);
	CTX_CB_ADD(session_checkuuid_get_cb);
	CTX_RETURN;
}

static void
session_check_cb(Zentific_Ctx *ctx, const char *configured_timeout){
	MARK_CB;
	DBG("(ctx=%p, timeout=%s)", ctx, configured_timeout);
	session_checker(ctx, configured_timeout, SES);
	RESUME;
}

Eina_Bool _checkSession(Zentific_Ctx *ctx){
	SESSION *session;
	DBG("(ctx=%p)", ctx);
	session = SES;
	if (!zshare_validate_uuid(session->session_id)){
		ctx->error = AZY_ERR(INVALID_SESSION);
		CTX_RETURN;
	}

	_getConfig(ctx, "session_timeout");
	CTX_CB_ADD(session_check_cb);
	CTX_RETURN;
}

static void
session_get_cb(Zentific_Ctx *ctx, SESSION *ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	if ((!ret) && azy_content_error_is_set(ERROR)){
		ctx->error = AZY_ERR(INVALID_SESSION);
	}
	if (!ctx->cbs) AZY_RETVAL(ret);
	NEXT_CB(ret);
}

Eina_Bool _getSession(Zentific_Ctx *ctx, const char *session_id){
	DBG("(ctx=%p, session_id=%s)", ctx, session_id);
	if ( !zshare_validate_uuid(session_id) ){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}

	db->getSession(db, ctx, session_id);
	CTX_CB_ADD(session_get_cb);
	CTX_RETURN;
}

Eina_Bool _removeSession(Zentific_Ctx *ctx, const char *session_id){
	DBG("(ctx=%p, session_id=%s)", ctx, session_id);
	if (!zshare_validate_uuid(session_id)){
		ctx->error = AZY_ERR(INVALID_SESSION);
		return EINA_FALSE;
	}
	db->removeSession(db, ctx, session_id);
	CTX_RETURN;
}

Eina_Bool _setLastLogin(Zentific_Ctx *ctx, int uid){
	DBG("(ctx=%p, uid=%i)", ctx, uid);
	if(uid < 0){
		ctx->error = AZY_ERR(INVALID_PARAMETER);
		return EINA_FALSE;
	}

	db->setLastLogin(db, ctx, uid);
	CTX_RETURN;
}

Eina_Bool _deauthSession(Zentific_Ctx *ctx, const char *session_id){
	DBG("(ctx=%p, session_id=%s)", ctx, session_id);
	if (!zshare_validate_uuid(session_id))
		ctx->error = AZY_ERR(INVALID_SESSION);
	else
		db->deauthSession(db, ctx, session_id);
	CTX_RETURN;
}

Eina_Bool _setSession(Zentific_Ctx *ctx){
	time_t now;
	SESSION *session;

	DBG("(ctx=%p)", ctx);
	session = SES;
	if (!session)
		return EINA_FALSE;

	now = time(NULL);

	/* time_t isnt an int, but loss of precision is fine unless
	 * this is still in use when I have grandchildren
	 */
	//INF("%i, %i", session->timestamp, (int)now);
	session->timestamp = (int)now;

	db->setSession(db, ctx, session);
	CTX_RETURN;
}


static void
session_pre_call_cb_super(Zentific_Ctx *ctx, int ret){
	MARK_CB;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	if (ctx->error){ /* should be impossible, but whatever */
		RESUME;
		return;
	}
	ctx->user_type = ret;
	//save session data
	//update database representation with new timestamp, etc
	_setSession(ctx);
	ctx->count = 0;
	ctx->checked = EINA_FALSE;
	CTX_CB_ADD(session_cb_error_clear);
}

static void
session_pre_call_cb_getSession(Zentific_Ctx *ctx, SESSION *ret){
	MARK_CB;
	SESSION *s = SES;
	DBG("(ctx=%p, ret=%p)", ctx, ret);
	if (!ret) INF("no session present!");
	else {
		s->authenticated = ret->authenticated;
		s->uid = ret->uid;
		eina_stringshare_replace(&s->referrer, ret->referrer);
		s->failures = ret->failures;
		s->timestamp = ret->timestamp;
		SESSION_free(ret);
	}
	RESUME;
}

static void
session_pre_call_cb_ip(Zentific_Ctx *ctx, const char *ip){
	MARK_CB;
	DBG("(ctx=%p, ip=%s)", ctx, ip);
	SES->client_host = eina_stringshare_add(ip);
	RESUME;
	/* calls session_pre_call() next */
}

/* hook for single-sign on using a proxy */
static void
session_pre_call_sso(Zentific_Ctx *ctx){
	SESSION *session;
	const char *str, *fwded_host, *fwded_server;
	int uid;

	if (!ctx->sso) return;
	session = SES;
	switch (ctx->count){
		case 1:
			_getConfig(ctx, "SSOtrustedhost");
			/* we're pretty baller, so we just come right back here with the value and skip precall */
			CTX_VALUE_JOB(session_pre_call_sso);
			break;
		case 2:
			str = ctx->data = CTX_PARAM_GET;
			if (!zshare_validate_ip(str)) goto error;
			fwded_host = azy_net_header_get(azy_server_module_net_get(ctx->parent), "X-Forwarded-Host");
			fwded_server = azy_net_header_get(azy_server_module_net_get(ctx->parent), "X-Forwarded-Server");
			if ((!fwded_server) && (!fwded_host)) goto error;
			/* FIXME: is checking for the presence of the SSO trusted host in X-Forwarded-For
			 * adequate, or do we need to specifically check the second-to-last item
			 * in the X-Forwarded-For list?
			 */
			if ((!strstr(fwded_server, str)) && (!strstr(fwded_host, str))){
				/* only process SSO if the header is being sent by the correct host */
				ERR("SSO not authorized; trusted host mismatch. session_id='%s' method='%s'",
					azy_server_module_session_get(ctx->parent), azy_content_method_full_get(ERROR));
				ctx->error = AZY_ERR(NOT_AUTHORIZED);
				goto error;
			}
			eina_stringshare_del(str);
			_getConfig(ctx, "SSOheader");
			CTX_VALUE_JOB(session_pre_call_sso);
			break;
		case 3:
			str = ctx->data = CTX_PARAM_GET;
			if (!str){
				/* only continue SSO if the header indicating the user has been provided */
				ERR("SSO not authorized: SSO user header not present. session_id='%s' method='%s'",
					azy_server_module_session_get(ctx->parent), azy_content_method_full_get(ERROR));
				ctx->error = AZY_ERR(NOT_AUTHORIZED);
				goto error;
			}
			_getUserUID(ctx, str);
			CTX_INT_JOB(session_pre_call_sso);
			break;
		case 4:
			SES->uid = uid = (intptr_t)CTX_PARAM_GET;
			if (uid < 0){
				/* bail SSO process if the header indicating user is absent */
				ERR("SSO not authorized: unknown SSO supplied. session_id='%s' method='%s'",
					azy_server_module_session_get(ctx->parent), azy_content_method_full_get(ERROR));
				ctx->error = AZY_ERR(NOT_AUTHORIZED);
				goto error;
			}
			if (!zshare_validate_uuid(SES->session_id)){
				const char *uuid;
				/* if invalid session (new cookie, first time hitting RPC after SSO auth),
				 *	then log some data
				 */
				INF("SSO session initiated via '%s' for user '%s'",
					azy_net_header_get(azy_server_module_net_get(ctx->parent), "X-Forwarded-Server"),
					(char*)ctx->data);
				uuid = azy_uuid_new();
				if (!session->session_id)
					INF("session_id not present for ip='%s', initiated new session_id='%s'", session->client_host, uuid);
				eina_stringshare_del(session->session_id);
				session->session_id = uuid;
				azy_server_module_session_set(ctx->parent, session->session_id);
			}
			eina_stringshare_del(ctx->data);
			/* TODO: should we bother reissuing new cookie (i.e., to mitigate
			 *	session-fixation/stealing attacks)?
			 *
			 *	For most redirect-driven federated-auth systems, the first
			 *	time zrpc ever sees a request, it will be first contact;
			 *	possible a long-lived cookie will hit us after SSO ticket
			 *	expires (then, we should re-issue).
			 *
			 * 	Also -- and very unlike User.login()  -- SSO
			 *	authentication of this style is almost always reiterated
			 *	with every request to the server.
			 *
			 * with SSO in this scheme, apache (or other) sits in front of
			 *	zentific resources and redirects to permit federated authentication
			 *	before zrpc is ever contacted to issue a first cookie..so the attack
			 *	surface here is a LOT smaller
			 *
			 *	session->session_id = getNewCookie(error);
			 *	_conn->session_id = g_strdup(session->session_id);
			 */
			session->authenticated = EINA_TRUE;
			_setSession(ctx);
			CTX_BOOL_JOB(session_pre_call_sso);
		case 5:
			if (ctx->error) goto error;
			/* because every contact with SSO is effectively a 'login' */
			_setLastLogin(ctx, session->uid);
		default:
			break;
	}
	return;
error:
	eina_stringshare_del(ctx->data);
	ctx->sso = EINA_FALSE;
	REWIND;
	RESUME;
}

static void
session_pre_call_sso_check(Zentific_Ctx *ctx){
	const char *sso;
	switch (ctx->count){
		case 0:
			_getConfig(ctx, "SSOenabled");
			CTX_VALUE_JOB(session_pre_call_sso_check);
			break;
		case 1:
			sso = CTX_PARAM_GET;
			if (!zshare_strcasecmp(sso, "yes"))
				ctx->sso = EINA_TRUE;
			eina_stringshare_del(sso);
			ctx->error = 0;
			if (!ctx->sso) {
				ctx->count++;
				REWIND;
				RESUME;
			}
		default:
			session_pre_call_sso(ctx);
	}
}

Eina_Bool session_pre_call(Zentific_Ctx *ctx, Azy_Net *to_client) {

	// Get supplied Cookie for session info, if any
	const char *cookie, *method = NULL;
	SESSION *session;
	Azy_Content *content;
	Eina_Bool fallthrough = EINA_FALSE;
	DBG("(ctx=%p, to_client=%p)", ctx, to_client);

	session = SES;
	content = ERROR;

	if (!session->client_host){
		/* #1 */
		session_getClientIP(ctx);
		CTX_CB_ADD(session_pre_call_cb_ip);
		REWIND; /* set up pre_call again */
		CTX_RETURN;
	}
	cookie = azy_server_module_session_get(ctx->parent);
	if (content) method = azy_content_method_full_get(content);
	if (cookie) DBG("found session_id='%s'", cookie);

	if (!zshare_validate_uuid(cookie)){
		const char *uuid;
		/* #2 */
		if (session->session_id)
			ERR("Bogus session_id='%s' passed from ip='%s'", session->session_id, session->client_host);
		if (content && (zshare_strcmp(method, "User.login") && zshare_strcmp(method, "Zentific.hello"))){
			/* this check doesn't matter for __download__ */
			if (!ctx->count){
				/* this will not come back if SSO authorizes */
				session_pre_call_sso_check(ctx);
				CTX_RETURN;
			}
			if (session->session_id)
				/* invalid uuid */
				ctx->error = AZY_ERR(INVALID_SESSION);
			else
				ctx->error = AZY_ERR(NOT_AUTHENTICATED);
			CTX_RETURN;
		}
		uuid = azy_uuid_new();
		if (!session->session_id)
			INF("session_id not present for ip='%s', initiated new session_id='%s'", session->client_host, uuid);
		eina_stringshare_del(session->session_id);
		session->session_id = uuid;
		azy_server_module_session_set(ctx->parent, session->session_id);
		_setSession(ctx);
		/* we're done with pre_call so DO NOT rewind */
		/* automatically passes the correct value to azy after db call returns */
		CTX_RETURN;
	}
	if (!ctx->checked){
		/* #2 */
		if (!ctx->count){
			/* this will not come back if SSO authorizes */
			session_pre_call_sso_check(ctx);
			CTX_RETURN;
		}
		if (!session->session_id){
			eina_stringshare_replace(&session->session_id, cookie);
			_getSession(ctx, session->session_id);
			CTX_CB_ADD(session_pre_call_cb_getSession);
		} else
			_checkSession(ctx);
		/* set up pre_call again for both calls */
		REWIND;
		CTX_RETURN;
	}

	/* #3 */

	if ((!zshare_strcmp(method, "User.login") || (!zshare_strcmp(method, "Zentific.hello")))){
		fallthrough = EINA_TRUE;
		/* reset for check in login later
		 * also resets after login call, but who cares
		 */
		RESUME;
		session->uid = -1;
		session->authenticated = EINA_FALSE;
	}
	else if (session->authenticated && (!zshare_strcmp(method, "User.logout")))
		fallthrough = EINA_TRUE;
	else if (azy_net_type_get(azy_server_module_net_get(ctx->parent)) == AZY_NET_TYPE_GET){
		INF("__download__ fallthrough for ip='%s' session_id='%s'", session->client_host, session->session_id);
		ctx->error = 0;
		RESUME;
		CTX_RETURN;
	}

	//TODO determine what other methods (besides User.login) can be
	//	called without being authenticated first
	/* this is called if no cookie is set in the header */
	if (fallthrough){
		INF("login/logout/hello fallthrough for ip='%s' session_id='%s'", session->client_host, session->session_id);
		ctx->error = 0;
		CTX_RETURN;
	}
	// actions upon result of checksession. if invalid session,  return false + error + please authenticate
	if (session->status  == AZY_ERRCODE(VALID_SESSION) ){
		/* flag superuser if necessary */
		_getUserType(ctx, session->uid);
		CTX_CB_ADD(session_pre_call_cb_super);
		/* done with pre_call, no rewind */
		INF("valid session_id='%s' from ip='%s' to method='%s'",
			session->session_id, session->client_host, method);
		ctx->error = 0;
		CTX_RETURN;
	}

	if (session->status == AZY_ERRCODE(EXPIRED_SESSION)) {
		ERR("expired session_id='%s'", session->session_id);
		ctx->error = AZY_ERR(EXPIRED_SESSION);
	} else if (session->status == AZY_ERRCODE(NOT_AUTHORIZED)) {
		ERR("not authorized session_id='%s' method='%s'", session->session_id, method);
		ctx->error = AZY_ERR(NOT_AUTHORIZED);
	} else {
		//NOTE: we may want to save session data here, also, because
		//      if sessions are pruned from db after expiry but client
		//      hasnt closed browser and sends a sessid, checkSession will return
		//      invalid (because it doesnt exist in the table) and drop us here.
		//	      (client would reauth and get new sessid, but potential
		//	      issue here if malicious user)
		//      This is significant because we may desire some variety of session tracking
		//      for unauthenticated hosts to try to prevent DOS attacks (e.g., by nailing us
		//      with tons of sessions from a given host, tying up processing power.) could do:
		//	      if num-failures > threshold, trigger network-block of host?
		//      or similar.

		//      Still cant evade many hosts hitting at same time, but this would at least help avoid
		//      a single or several evil hosts


		//happens if ( status == INVALID_SESSION ) or other
		//FIXME what about invalid sessions which are calling methods which do not require authentication?
		ctx->error = AZY_ERR(INVALID_SESSION);
	}

	CTX_RETURN;
}
