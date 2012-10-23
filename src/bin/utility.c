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
#include "zentific.h"

#include <openssl/ssl.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "utility.h"

/*
steve@eee:/tmp$ openssl genrsa  -out mykey.pem 1024
steve@eee:/tmp$ openssl rsa -in mykey.pem -pubout > mykeypub.pem

 or for passwd protected

steve@eee:/tmp$ openssl genrsa -des3 -out mykey.pem 1024
steve@eee:/tmp$ openssl rsa -in mykey.pem -pubout > mykeypub.pem
*/

static void _openssl_print_errors(void) {
	unsigned long err;
   	do {
		err = ERR_get_error();
		if (!err) break;
		ERR("credential store openssl error: '%s'", ERR_reason_error_string(err));
	} while (1);
}

unsigned char * _encryptString(const char *keypath, const char *input, int *enc_size) {

	RSA *rsa = NULL;
	FILE *pub = NULL;
	unsigned char *enc = NULL;

	/* declared only as place-keepers for now */
	int (*password_callback)(char *, int,  int,  void *) = NULL;
	char *password = NULL;

	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();

	/* TODO consider adding: RSA_blinding_on(RSA *r, BN_CTX *x) */

	if(!keypath){
		ERR("invalid path parameter");
		goto error;
	}

	if(!input){
		ERR("invalid input string");
		goto error;
	}
	if (!(pub = fopen(keypath, "r"))){
		ERR("could not open pub key");
		goto error;
	}

	if(!(rsa = PEM_read_RSA_PUBKEY(pub, NULL, password_callback, password))){
		ERR("could not read pub key");
		goto error;
	}

	fclose(pub);
	pub = NULL;

	if(!(enc = malloc(RSA_size(rsa)))){
		ERR("allocation failed");
		goto error;
	}

	*enc_size = RSA_public_encrypt(strlen(input), (unsigned char*)input, enc, rsa, RSA_PKCS1_OAEP_PADDING);

	if (*enc_size < 0)
		goto error;

	RSA_free(rsa);

	return enc;
error:
	_openssl_print_errors();

	if (pub)
		fclose(pub);
	if (rsa)
		RSA_free(rsa);

		free(enc);
	return NULL;
}

