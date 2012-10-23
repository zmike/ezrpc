#ifndef Z_DEFINES_H
#define Z_DEFINES_H
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
//---------------DEFINES--------------------

//Zentific formalities
#define ZENTIFIC_XMLRPC_VER     PACKAGE_VERSION
#define ZENTIFIC_COPYRIGHT      "Copyright 2007, 2008, 2009, 2010, 2011, Zentific LLC and AUTHORS"
#define ZENTIFIC_DISCLAIMER     "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS.' ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
#define ZENTIFIC_CONTACT	"Please send general correspondence to contact@zentific.com and bug reports to bugs@zentific.com"

//Zentific default presets
#define DEFAULT_CONF_FILE 	PREFIX"/zrpc.conf"			// default path to the zentific config file
#define DEFAULT_PID_FILE 	"zrpc.pid"	 			// file location to store process id
#define DEFAULT_UTILS_PATH 	PREFIX"/bin"  	       		// path to utilities directory
#define DEFAULT_VAR_PATH 	"/var"	 				// path to variable data directory
#define DEFAULT_SSH_PATH	"/usr/bin/ssh"				// path to ssh exe

#define DEFAULT_MODULES_PATH 	PREFIX"/modules" 			// path to zrpc modules directory

#define DEFAULT_DB_MODULES_PATH	PREFIX"/db"    			// path to db modules directory
#define DEFAULT_DB_MODULE 	"zrpcdb"				// default db module to load
#define DEFAULT_CONNS	   5
#define DEFAULT_LOG_FILE 	PREFIX"/logs/zrpc.log"		// file location to store log file
#define DEFAULT_LOG_TYPE 	ZSHARE_LOG_TYPE_FILE			// default type of logging destination
									//   (remote is not now but will be supported)
#define DEFAULT_PORT	 	4444				 	// default port to which server binds
#define DEFAULT_LISTEN	 	"127.0.0.1"			    	// default address upon which server listens
#define DEFAULT_KEY_PATH	PREFIX"/ssh"				// default path to trust
#define DEFAULT_SSL_PATH	PREFIX"/ssl"				// default path to trust
#define DEFAULT_SSL_CERT	"server.pem"				// default path to trust
#define IGNORE_SSL_ERROR	1					// ignore trust errors (e.g. expired or self-signed certs)
#define DEFAULT_WEB_ROOT	"/opt/www/zentific.com"
#define DEFAULT_MAX_LOGIN_FAILURES	5 				//max login failures before temporary lockout specified by DEFAULT_RETRY_TIMEOUT
#define DEFAULT_VM_CONF_PATH	PREFIX"/vms"
#define ZRPC_SERVLET_PATH	LIBDIR				//path for zrpc servlet modules

#define MAX_USERNAME_LENGTH	255
#define DEFAULT_RETRY_TIMEOUT	5					//minutes before logins are again permitted after max_failed_logins was reached
#define DEFAULT_SESSION_TIMEOUT	15					//minutes of inactivity before login timeout
#define DEFAULT_SALT_LENGTH	4					//length of salt used in password hashing

#define MIN_PASSWD_LEN		8					// minimum password length

#define DEFAULT_CREDSTORE_PUBKEY_PATH	PREFIX"/credstore-pubkey.pem"	//public RSA key for our credential store

//----------COMPILE TIME TWEAKS-------------
#define SESSION_PROXY_SUPPORT	1

#define DEFAULT_ZENSCHED_PORT   18000

#define ZENSCHED_PROTO_IDENT		"ZENTIFICSCHEDULER"
#define ZENSCHED_PROTO_NEWJOB		"ZENTIFIC_NEWJOB"
#define ZENSCHED_PROTO_CANCELJOB	"ZENTIFIC_CANCELJOB"
#define ZENSCHED_PROTO_RESPONSE		"ZOKAY"
#define ZENSCHED_PROTO_CANCELJOBMSG	ZENSCHED_PROTO_IDENT":::"ZENSCHED_PROTO_CANCELJOB
#define ZENSCHED_PROTO_NEWJOBMSG	ZENSCHED_PROTO_IDENT":::"ZENSCHED_PROTO_NEWJOB
#endif
