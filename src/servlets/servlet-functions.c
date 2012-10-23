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
#include "ztypes.h"
#include "zentific-defines.h"
#include "functions-session.h"
#include "functions-user.h"
#include "functions-auth.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <inttypes.h>

static void
login_cb_uid(Zentific_Ctx *ctx, int ret){
	SESSION *session;
	MARK_CB;
	session = SES;
	DBG("(ctx=%p, ret=%i)", ctx, ret);
	if (ret < 0){
		/* invalid username */
		if (ctx->error == AZY_ERR(UNKNOWN_RESOURCE))
			/* Suppressing an error here is very intentional. We do not want to leak
			 *	information about valid or invalid usernames
			 */
			ctx->error = 0;
		else if (ctx->error) {
			/* something crazy happened */
			ERR("%s", eina_error_msg_get(ctx->error));
			RESUME;
			return;
		}
		/* skip to setting session, but set ret value here */
		AZY_RETVAL(NULL);
		_setSession(ctx);
		return;
	}
	session->uid = ret;
	REWIND;
	RESUME;
}

Eina_Bool _login(Zentific_Ctx *ctx, const char *user, const char *pass){
	zentific_config *zconf = CFG;
	SESSION *session = SES;
	//merge with auth() so we can just return it?

	/* can only be set from _getUserUID below (explicitly set in pre_call */
	if (session->uid == -1) {
		/* #1, suspended */
		//FIXME isActive isUser should also go here
		eina_stringshare_replace(&session->username, user);
		if (session->failures >= zconf->MAX_LOGIN_FAILURES) {
			time_t now;
			now = time(NULL);

			if ( now - session->timestamp < zconf->RETRY_TIMEOUT * 60) {
				ctx->error = AZY_ERR(MAX_LOGIN_FAILURES);
				CTX_RETURN;
			} else {
				//failure timeout has passed, reset counter
				session->failures = 0;
			}
		}

		if(eina_strlen_bounded(user, MAX_USERNAME_LENGTH + 1) > MAX_USERNAME_LENGTH){
			ctx->error = AZY_ERR(INVALID_PARAMETER);
			CTX_RETURN;
		}
		_getActiveUserUID(ctx, user);
		CTX_CB_ADD(login_cb_uid);
		CTX_RETURN;
	}
	//FIXME important:
	//	need an 'available authentication mechanisms'
	//	array, with function pointers, ending with NULL
	//	-loop through. if one is true, break and return
	//	else fall through and return false

	/* #2, NOT suspended */
	if (!ctx->auth){
		if (!_auth(ctx, pass)){
			if (!azy_content_error_is_set(ERROR))
				ctx->error = AZY_ERR(SERVER_FAILURE);
			CTX_RETURN;
		}
		if (!ctx->auth) CTX_RETURN;
	}
	if (session->authenticated) {
		session->failures = 0;
		_setLastLogin(ctx, session->uid);
		CTX_CB_ADD(NULL);
		eina_stringshare_replace(&session->session_id, azy_uuid_new());
		azy_server_module_session_set(ctx->parent, session->session_id);

		INF("Login status='success' user='%s' host='%s' issued new session_id='%s'",
				session->username, session->client_host, session->session_id);
	} else {
		session->failures++;
		if (azy_content_error_is_set(ERROR) && (azy_content_error_code_get(ERROR) != AZY_ERR(DB_FAIL)))
			azy_content_error_reset(ERROR);
		WRN("Login status='failure' user='%s' host='%s' session_id='%s'",
				session->username, session->client_host, session->session_id);
	}
	/* #3, NOT suspended */
	_setSession(ctx);
	/* automatically returns correct value to azy with setSession */
	return session->authenticated;
}

void _servlet_download(Server_Module *data_) {
	Zentific_Ctx *ctx = &data_->ctx;
	Azy_Net *net_ = azy_server_module_net_get(ctx->parent);
	Azy_Server_Module *module = ctx->parent;
	SESSION *session = data_->session;
	Eina_Strbuf *filepath;
	const char *path, *cookie, *file, *contenttype = NULL;
	FILE *f;
	struct stat st;
	char fbuf[PATH_MAX];
	int fd;
	void *mem;

	//if we are not running in self-hosting mode,
	//	dont even entertain the notion and bail
	if ((!data_) || (!data_->zconf) || (!data_->zconf->SELF_HOST)){
		INF("Refusing to send files");
		azy_server_module_events_resume(module, EINA_FALSE);
		return;
	}

	path = azy_net_uri_get(net_);
	cookie = azy_server_module_session_get(module);

	if (cookie && zshare_validate_uuid(cookie)){
#ifdef ZDEV_DEBUG
	//DBG("sessid = %s", cookie);
#endif
		session->session_id = eina_stringshare_add(cookie);
	}

	if (strstr(path, "login"))
		session->referrer = eina_stringshare_add("/Zentific/index.html");
	else
		session->referrer = eina_stringshare_ref(path);

	/* FIXME this is a really bad way of making sure login works. too encompassing.
	 * make /login/ a sort of web chroot w/ duplicate or symlinked js?
	 */
	//OLD if ( status != VALID_SESSION && zshare_strcmp( strrchr(path, '/'), "/index.html" ) != 0 ) {
	if ((zshare_strcmp(strrchr(path, '/'), "/index.html" ))
		&& ((!strstr(path, "/javascript/")) && (!strstr(path, "/images/"))
		&& (!strstr(path, "/plugins/")) && (!strstr(path, "/sansation-webfont/"))
		&& (!strstr(path, "/css/")) && (!strstr(path, "/icon_set/"))
#ifdef ZDEV_DEBUG
		&& (!strstr(path, "/debug/"))
#endif
		)) {
		//redirect to login page
		INF("Sending 302 redirect");
		azy_net_code_set(net_, 302);
		azy_net_header_set(net_, "Location", "index.html");

		azy_server_module_send(module, net_, NULL);
		azy_server_module_events_resume(module, EINA_TRUE);
		return;
	}

	//FIXME if auth fails use 401 Not Authorized?
	//	might want to present a different error page perhaps.

	filepath = eina_strbuf_new();
	eina_strbuf_append_printf(filepath, "%s%s", data_->zconf->WEB_ROOT, path);


	//FIXME
	/* canonicalize_file_name

	// constructed_path = Z_WEB_ROOT + path
	//	canonpath = get_canonical_path(constructed path)
	//	if ((dirname(canonpath) == constructed_path - path) && g_path_is_absolute(canon_path) && g_path_is_absolute(constructed_path)

	1) on windows the '\' characters are first replaced with '/'
	2) we use g_path_is_absolute to check if the path is absolute, if it is already jump to number 4
	3) g_path_get_dirname is used to get the current directory and it is combined with relative path
	4) all instances of "./" are simply removed
	5) if there's any instances of "/../" we search back to the last '/' character and
		remove everything in between (including the "/../" instance)
	6) if there is no last '/' character when searching backwards the path
		is in form "/tmp/../../" or "C:/tmp/../../" in which case the "../" can simply be removed
	7) the conversion is done, can be done in one function quite easily, no need to mess with
		windows api that is suggested, works on all platforms


	*/

//FIXME language negotiation here (to serve correct file by ISO language code e.g. en_US.xml)

	//FIXME, perhaps use a legit HTTP code like 301 moved permanently for this?
	//	not a big deal, really

	file = eina_strbuf_string_get(filepath);
	if (file[eina_strbuf_length_get(filepath) - 1] == '/')
		eina_strbuf_append(filepath, "index.html");

	if ( eina_str_has_suffix(file, ".css"))
		contenttype = "text/css";
	else if ( eina_str_has_suffix(file, ".js") || eina_str_has_suffix(file, ".ctf"))
		contenttype = "application/javascript";
	else if ( eina_str_has_suffix(file, ".gz"))
		contenttype = "application/x-gzip";
	else if ( eina_str_has_suffix(file, ".gif"))
		contenttype = "image/gif";
	else if (eina_str_has_suffix(file, ".png"))
		contenttype = "image/png";
	else if (eina_str_has_suffix(file, ".ttf"))
		contenttype = "application/x-font-ttf";
	else if (eina_str_has_suffix(file, ".eot"))
		contenttype = "application/vnd.ms-fontobject";
	else if (eina_str_has_suffix(file, ".svg"))
		contenttype = "image/svg+xml";
	else if ( eina_str_has_suffix(file, ".xml") || eina_str_has_suffix(file, ".xsl") )
		contenttype = "application/xml";
	else if (eina_str_has_suffix(file, ".woff"))
		contenttype = "application/x-woff";
	else if (eina_str_has_suffix(file, ".mov"))
		contenttype = "video/quicktime";
	else if (eina_str_has_suffix(file, ".jar"))
		contenttype = "application/java-archive";
	else
		contenttype = "text/html";

	if (!(f = fopen(file, "rb"))){
err:
		//FIXME - more granular error handling? e.g. throw a 403 forbidden if permissions arent correct, etc

		//FIXME g_file_get_contents to a generic 404 page styled like the rest of zentific?
		//g_string_append_printf(response,
		//	"<html><head><title>404 Not Found</title></head>"
		//	"<body><h1>404 Not Found</h1><p>You are not authorized to download %s</p></body></html>",
		//	path);
		ERR("path %s, filepath %s: error %s", path, file, strerror(errno));
		eina_strbuf_string_free(filepath);
		if (ctx->count) {
			Azy_Net_Data nd;
			eina_strbuf_free(filepath);
			INF("Sending 404 error");
			nd.data = (unsigned char*)"404 error\n\r";
			nd.size = sizeof("404 error\n\r") - 1;
			azy_server_module_send(module, net_, &nd);
			azy_server_module_events_resume(module, EINA_TRUE);
			return;
		}
		//FIXME: change these to 404.html on release
		eina_strbuf_append_printf(filepath, "%s/404.html", data_->zconf->WEB_ROOT);
		f = fopen(file, "rb");
		azy_net_code_set(net_, 404);
		ctx->count++;
	} else
		azy_net_code_set(net_, 200);

	snprintf(fbuf, sizeof(fbuf), "%s/404.html", data_->zconf->WEB_ROOT);
	if ((!f) && (!zshare_strcmp(file, fbuf))){
		Azy_Net_Data nd;
		eina_strbuf_free(filepath);
		INF("Sending 404 error");
		nd.data = (unsigned char*)"404 error\n\r";
		nd.size = sizeof("404 error\n\r") - 1;
		azy_net_header_set(net_, "Content-Type", "text/plain");
		azy_server_module_send(module, net_, &nd);
		azy_server_module_events_resume(module, EINA_TRUE);
		return;
	}
	else if (!f)
		goto err;
	fd = fileno(f);
	if (fstat(fd, &st)){
		fclose(f);
		goto err;
	}
	if ((mem = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED){
		fclose(f);
		goto err;
	}
	INF("Sending file %s, %"PRIu64" bytes", file, (uintmax_t)st.st_size);
	azy_net_header_set(net_, "Content-Type", contenttype);
	//azy_net_header_set(net_, "Content-Encoding", "x-gzip");
	{
		Azy_Net_Data nd;
		nd.data = mem;
		nd.size = st.st_size;
		azy_server_module_send(module, net_, &nd);
	}
	munmap(mem, st.st_size);
	fclose(f);
	eina_strbuf_free(filepath);
	azy_server_module_events_resume(module, EINA_TRUE);
	return;
}
