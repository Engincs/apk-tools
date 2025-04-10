/*	$NetBSD: fetch.c,v 1.19 2009/08/11 20:48:06 joerg Exp $	*/
/*-
 * Copyright (c) 1998-2004 Dag-Erling Coïdan Smørgrav
 * Copyright (c) 2008 Joerg Sonnenberger <joerg@NetBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD: fetch.c,v 1.41 2007/12/19 00:26:36 des Exp $
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fetch.h"
#include "common.h"

fetch_redirect_t fetchRedirectMethod;
auth_t	 fetchAuthMethod;
struct fetch_error fetchLastErrCode;
int	 fetchTimeout;
volatile int	 fetchRestartCalls = 1;
int	 fetchDebug;


/*** Public API **************************************************************/

/*
 * Select the appropriate protocol for the URL scheme, and return a
 * read-only stream connected to the document referenced by the URL.
 * Also fill out the struct url_stat.
 */
fetchIO *
fetchXGet(struct url *URL, struct url_stat *us, const char *flags)
{

	if (us != NULL) {
		us->size = -1;
		us->atime = us->mtime = 0;
	}
	if (strcasecmp(URL->scheme, SCHEME_HTTP) == 0)
		return (fetchXGetHTTP(URL, us, flags));
	else if (strcasecmp(URL->scheme, SCHEME_HTTPS) == 0)
		return (fetchXGetHTTP(URL, us, flags));
	url_seterr(URL_BAD_SCHEME);
	return (NULL);
}

/*
 * Select the appropriate protocol for the URL scheme, and return a
 * read-only stream connected to the document referenced by the URL.
 */
fetchIO *
fetchGet(struct url *URL, const char *flags)
{
	return (fetchXGet(URL, NULL, flags));
}

/*
 * Select the appropriate protocol for the URL scheme, and return a
 * write-only stream connected to the document referenced by the URL.
 */
fetchIO *
fetchPut(struct url *URL, const char *flags)
{

	if (strcasecmp(URL->scheme, SCHEME_HTTP) == 0)
		return (fetchPutHTTP(URL, flags));
	else if (strcasecmp(URL->scheme, SCHEME_HTTPS) == 0)
		return (fetchPutHTTP(URL, flags));
	url_seterr(URL_BAD_SCHEME);
	return (NULL);
}

/*
 * Select the appropriate protocol for the URL scheme, and return the
 * size of the document referenced by the URL if it exists.
 */
int
fetchStat(struct url *URL, struct url_stat *us, const char *flags)
{

	if (us != NULL) {
		us->size = -1;
		us->atime = us->mtime = 0;
	}
	if (strcasecmp(URL->scheme, SCHEME_HTTP) == 0)
		return (fetchStatHTTP(URL, us, flags));
	else if (strcasecmp(URL->scheme, SCHEME_HTTPS) == 0)
		return (fetchStatHTTP(URL, us, flags));
	url_seterr(URL_BAD_SCHEME);
	return (-1);
}

/*
 * Select the appropriate protocol for the URL scheme, and return a
 * list of files in the directory pointed to by the URL.
 */
int
fetchList(struct url_list *ue, struct url *URL, const char *pattern,
    const char *flags)
{

	if (strcasecmp(URL->scheme, SCHEME_HTTP) == 0)
		return (fetchListHTTP(ue, URL, pattern, flags));
	else if (strcasecmp(URL->scheme, SCHEME_HTTPS) == 0)
		return (fetchListHTTP(ue, URL, pattern, flags));
	url_seterr(URL_BAD_SCHEME);
	return -1;
}

/*
 * Attempt to parse the given URL; if successful, call fetchXGet().
 */
fetchIO *
fetchXGetURL(const char *URL, struct url_stat *us, const char *flags)
{
	struct url *u;
	fetchIO *f;

	if ((u = fetchParseURL(URL)) == NULL)
		return (NULL);

	f = fetchXGet(u, us, flags);

	fetchFreeURL(u);
	return (f);
}

/*
 * Attempt to parse the given URL; if successful, call fetchGet().
 */
fetchIO *
fetchGetURL(const char *URL, const char *flags)
{
	return (fetchXGetURL(URL, NULL, flags));
}

/*
 * Attempt to parse the given URL; if successful, call fetchPut().
 */
fetchIO *
fetchPutURL(const char *URL, const char *flags)
{
	struct url *u;
	fetchIO *f;

	if ((u = fetchParseURL(URL)) == NULL)
		return (NULL);

	f = fetchPut(u, flags);

	fetchFreeURL(u);
	return (f);
}

/*
 * Attempt to parse the given URL; if successful, call fetchStat().
 */
int
fetchStatURL(const char *URL, struct url_stat *us, const char *flags)
{
	struct url *u;
	int s;

	if ((u = fetchParseURL(URL)) == NULL)
		return (-1);

	s = fetchStat(u, us, flags);

	fetchFreeURL(u);
	return (s);
}

/*
 * Attempt to parse the given URL; if successful, call fetchList().
 */
int
fetchListURL(struct url_list *ue, const char *URL, const char *pattern,
    const char *flags)
{
	struct url *u;
	int rv;

	if ((u = fetchParseURL(URL)) == NULL)
		return -1;

	rv = fetchList(ue, u, pattern, flags);

	fetchFreeURL(u);
	return rv;
}

/*
 * Make a URL
 */
struct url *
fetchMakeURL(const char *scheme, const char *host, int port, const char *doc,
    const char *user, const char *pwd)
{
	struct url *u;

	if (!scheme || (!host && !doc)) {
		url_seterr(URL_MALFORMED);
		return (NULL);
	}

	if (port < 0 || port > 65535) {
		url_seterr(URL_BAD_PORT);
		return (NULL);
	}

	/* allocate struct url */
	if ((u = calloc(1, sizeof(*u))) == NULL) {
		fetch_syserr();
		return (NULL);
	}

	if ((u->doc = strdup(doc ? doc : "/")) == NULL) {
		fetch_syserr();
		free(u);
		return (NULL);
	}

#define seturl(x) snprintf(u->x, sizeof(u->x), "%s", x)
	seturl(scheme);
	seturl(host);
	seturl(user);
	seturl(pwd);
#undef seturl
	u->port = port;

	return (u);
}

int
fetch_urlpath_safe(char x)
{
	if ((x >= '0' && x <= '9') || (x >= 'A' && x <= 'Z') ||
	    (x >= 'a' && x <= 'z'))
		return 1;

	switch (x) {
	case '$':
	case '-':
	case '_':
	case '.':
	case '+':
	case '!':
	case '*':
	case '\'':
	case '(':
	case ')':
	case ',':
	/* The following are allowed in segment and path components: */
	case '?':
	case ':':
	case '@':
	case '&':
	case '=':
	case '/':
	case ';':
	/* If something is already quoted... */
	case '%':
		return 1;
	default:
		return 0;
	}
}

/*
 * Copy an existing URL.
 */
struct url *
fetchCopyURL(const struct url *src)
{
	struct url *dst;
	char *doc;

	/* allocate struct url */
	if ((dst = malloc(sizeof(*dst))) == NULL) {
		fetch_syserr();
		return (NULL);
	}
	if ((doc = strdup(src->doc)) == NULL) {
		fetch_syserr();
		free(dst);
		return (NULL);
	}
	*dst = *src;
	dst->doc = doc;

	return dst;
}

/*
 * Return value of the given hex digit.
 */
static int
fetch_hexval(char ch)
{
	if (ch >= '0' && ch <= '9')
		return (ch - '0');
	else if (ch >= 'a' && ch <= 'f')
		return (ch - 'a' + 10);
	else if (ch >= 'A' && ch <= 'F')
		return (ch - 'A' + 10);
	return (-1);
}

/*
 * Decode percent-encoded URL component from src into dst, stopping at end
 * of string or one of the characters contained in brk.  Returns a pointer
 * to the unhandled part of the input string (null terminator, specified
 * character).  No terminator is written to dst (it is the caller's
 * responsibility).
 */
static const char *
fetch_pctdecode(char *dst, const char *src, const char *brk, size_t dlen)
{
	int d1, d2;
	char c;
	const char *s;

	for (s = src; *s != '\0' && !strchr(brk, *s); s++) {
		if (s[0] == '%' && (d1 = fetch_hexval(s[1])) >= 0 &&
		    (d2 = fetch_hexval(s[2])) >= 0 && (d1 > 0 || d2 > 0)) {
			c = d1 << 4 | d2;
			s += 2;
		} else if (s[0] == '%') {
			/* Invalid escape sequence. */
			return (NULL);
		} else {
			c = *s;
		}
		if (!dlen)
			return NULL;
		dlen--;
		*dst++ = c;
	}
	return (s);
}

/*
 * Split a URL into components. URL syntax is:
 * [method:/][/[user[:pwd]@]host[:port]/][document]
 * This almost, but not quite, RFC1738 URL syntax.
 */
struct url *
fetchParseURL(const char *URL)
{
	const char *p, *q;
	struct url *u;
	size_t i, count;
	int pre_quoted;

	/* allocate struct url */
	if ((u = calloc(1, sizeof(*u))) == NULL) {
		fetch_syserr();
		return (NULL);
	}

	if (*URL == '/' || strncmp(URL, "file:", 5) == 0) {
		url_seterr(URL_BAD_SCHEME);
		goto ouch;
	}
	if (strncmp(URL, "http:", 5) == 0 ||
	    strncmp(URL, "https:", 6) == 0) {
		pre_quoted = 1;
		if (URL[4] == ':') {
			strcpy(u->scheme, SCHEME_HTTP);
			URL += 5;
		} else {
			strcpy(u->scheme, SCHEME_HTTPS);
			URL += 6;
		}

		if (URL[0] != '/' || URL[1] != '/') {
			url_seterr(URL_MALFORMED);
			goto ouch;
		}
		URL += 2;
		p = URL;
		goto find_user;
	}

	url_seterr(URL_BAD_SCHEME);
	goto ouch;

find_user:
	p = strpbrk(URL, "/@");
	if (p != NULL && *p == '@') {
		/* username */
		q = URL;
		q = fetch_pctdecode(u->user, q, ":@", URL_USERLEN);
		if (q == NULL) {
			url_seterr(URL_BAD_AUTH);
			goto ouch;
		}

		/* password */
		if (*q == ':') {
			q = fetch_pctdecode(u->pwd, q + 1, "@", URL_PWDLEN);
			if (q == NULL) {
				url_seterr(URL_BAD_AUTH);
				goto ouch;
			}
		}
		if (*q != '@') {
			url_seterr(URL_BAD_AUTH);
			goto ouch;
		}
		p++;
	} else {
		p = URL;
	}

	/* hostname */
	if (*p == '[' && (q = strchr(p + 1, ']')) != NULL &&
	    (*++q == '\0' || *q == '/' || *q == ':')) {
		if ((i = q - p - 2) >= URL_HOSTLEN) {
			url_seterr(URL_BAD_HOST);
			goto ouch;
		}
		strncpy(u->host, ++p, i);
		p = q;
	} else {
		for (i = 0; *p && (*p != '/') && (*p != ':'); p++) {
			if (i >= URL_HOSTLEN) {
				url_seterr(URL_BAD_HOST);
				goto ouch;
			}
			u->host[i++] = *p;
		}
	}

	/* port */
	if (*p == ':') {
		u->port = fetch_parseuint(p + 1, &p, 10, IPPORT_MAX);
		if (*p && *p != '/') {
			/* invalid port */
			url_seterr(URL_BAD_PORT);
			goto ouch;
		}
	}

	/* document */
	if (!*p)
		p = "/";

	count = 1;
	for (i = 0; p[i] != '\0'; ++i) {
		if ((!pre_quoted && p[i] == '%') ||
		    !fetch_urlpath_safe(p[i]))
			count += 3;
		else
			++count;
	}

	if ((u->doc = malloc(count)) == NULL) {
		fetch_syserr();
		goto ouch;
	}
	for (i = 0; *p != '\0'; ++p) {
		if ((!pre_quoted && *p == '%') ||
		    !fetch_urlpath_safe(*p)) {
			u->doc[i++] = '%';
			if ((unsigned char)*p < 160)
				u->doc[i++] = '0' + ((unsigned char)*p) / 16;
			else
				u->doc[i++] = 'a' - 10 + ((unsigned char)*p) / 16;
			if ((unsigned char)*p % 16 < 10)
				u->doc[i++] = '0' + ((unsigned char)*p) % 16;
			else
				u->doc[i++] = 'a' - 10 + ((unsigned char)*p) % 16;
		} else
			u->doc[i++] = *p;
	}
	u->doc[i] = '\0';

	return (u);

ouch:
	free(u);
	return (NULL);
}

/*
 * Free a URL
 */
void
fetchFreeURL(struct url *u)
{
	free(u->doc);
	free(u);
}

static char
xdigit2digit(char digit)
{
	digit = tolower((unsigned char)digit);
	if (digit >= 'a' && digit <= 'f')
		digit = digit - 'a' + 10;
	else
		digit = digit - '0';

	return digit;
}

/*
 * Unquote whole URL.
 * Skips optional parts like query or fragment identifier.
 */ 
char *
fetchUnquotePath(struct url *url)
{
	char *unquoted;
	const char *iter;
	size_t i;

	if ((unquoted = malloc(strlen(url->doc) + 1)) == NULL)
		return NULL;

	for (i = 0, iter = url->doc; *iter != '\0'; ++iter) {
		if (*iter == '#' || *iter == '?')
			break;
		if (iter[0] != '%' ||
		    !isxdigit((unsigned char)iter[1]) ||
		    !isxdigit((unsigned char)iter[2])) {
			unquoted[i++] = *iter;
			continue;
		}
		unquoted[i++] = xdigit2digit(iter[1]) * 16 +
		    xdigit2digit(iter[2]);
		iter += 2;
	}
	unquoted[i] = '\0';
	return unquoted;
}


/*
 * Extract the file name component of a URL.
 */
char *
fetchUnquoteFilename(struct url *url)
{
	char *unquoted, *filename;
	const char *last_slash;

	if ((unquoted = fetchUnquotePath(url)) == NULL)
		return NULL;

	if ((last_slash = strrchr(unquoted, '/')) == NULL)
		return unquoted;
	filename = strdup(last_slash + 1);
	free(unquoted);
	return filename;
}

char *
fetchStringifyURL(const struct url *url)
{
	size_t total;
	char *doc;

	/* scheme :// user : pwd @ host :port doc */
	total = strlen(url->scheme) + 3 + strlen(url->user) + 1 +
	    strlen(url->pwd) + 1 + strlen(url->host) + 6 + strlen(url->doc) + 1;
	if ((doc = malloc(total)) == NULL)
		return NULL;
	if (url->port != 0)
		snprintf(doc, total, "%s%s%s%s%s%s%s:%d%s",
		    url->scheme,
		    url->scheme[0] != '\0' ? "://" : "",
		    url->user,
		    url->pwd[0] != '\0' ? ":" : "",
		    url->pwd,
		    url->user[0] != '\0' || url->pwd[0] != '\0' ? "@" : "",
		    url->host,
		    (int)url->port,
		    url->doc);
	else {
		snprintf(doc, total, "%s%s%s%s%s%s%s%s",
		    url->scheme,
		    url->scheme[0] != '\0' ? "://" : "",
		    url->user,
		    url->pwd[0] != '\0' ? ":" : "",
		    url->pwd,
		    url->user[0] != '\0' || url->pwd[0] != '\0' ? "@" : "",
		    url->host,
		    url->doc);
	}
	return doc;
}
