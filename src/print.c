/* print.c - Alpine Package Keeper (APK)
 *
 * Copyright (C) 2005-2008 Natanael Copa <n@tanael.org>
 * Copyright (C) 2008-2011 Timo Teräs <timo.teras@iki.fi>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "apk_defines.h"
#include "apk_print.h"
#include "apk_io.h"

#define DECLARE_ERRMSGS(func) \
	func(APKE_FILE_UNCHANGED,		"file is unchanged") \
	func(APKE_EOF,				"unexpected end of file") \
	func(APKE_DNS_FAIL,			"DNS: non-recoverable failure") \
	func(APKE_DNS_ADDRESS_FAMILY,		"DNS: address family for host not supported") \
	func(APKE_DNS_AGAIN,			"DNS: transient error (try again later)") \
	func(APKE_DNS_NO_DATA,			"DNS: no address for host") \
	func(APKE_DNS_NO_NAME,			"DNS: name does not exist") \
	func(APKE_TLS_ERROR,			"TLS: unspecified error") \
	func(APKE_TLS_SERVER_CERT_HOSTNAME,	"TLS: server hostname mismatch") \
	func(APKE_TLS_SERVER_CERT_UNTRUSTED,	"TLS: server certificate not trusted") \
	func(APKE_TLS_CLIENT_CERT_UNTRUSTED,	"TLS: client certificate not trusted") \
	func(APKE_TLS_HANDSHAKE,		"TLS: handshake failed (client cert needed?)") \
	func(APKE_URL_FORMAT,			"invalid URL (check your repositories file)") \
	func(APKE_HTTP_400_BAD_REQUEST,		"HTTP 400: Bad Request" ) \
	func(APKE_HTTP_401_UNAUTHORIZED,	"HTTP 401: Unauthorized" ) \
	func(APKE_HTTP_403_FORBIDDEN,		"HTTP 403: Forbidden" ) \
	func(APKE_HTTP_404_NOT_FOUND,		"HTTP 404: Not Found" ) \
	func(APKE_HTTP_405_METHOD_NOT_ALLOWED,	"HTTP 405: Method Not Allowed" ) \
	func(APKE_HTTP_406_NOT_ACCEPTABLE,	"HTTP 406: Not Acceptable" ) \
	func(APKE_HTTP_407_PROXY_AUTH_REQUIRED,	"HTTP 407: Proxy Authentication Required" ) \
	func(APKE_HTTP_408_TIMEOUT,		"HTTP 408: Timeout" ) \
	func(APKE_HTTP_500_INTERNAL_SERVER_ERROR, "HTTP 500: Internal Server Error" ) \
	func(APKE_HTTP_501_NOT_IMPLEMENTED,	"HTTP 501: Not Implemented" ) \
	func(APKE_HTTP_502_BAD_GATEWAY,		"HTTP 502: Bad Gateway" ) \
	func(APKE_HTTP_503_SERVICE_UNAVAILABLE,	"HTTP 503: Service Unavailable" ) \
	func(APKE_HTTP_504_GATEWAY_TIMEOUT,	"HTTP 504: Gateway Timeout" ) \
	func(APKE_HTTP_UNKNOWN,			"HTTP: unrecognized server error" ) \
	func(APKE_CRYPTO_ERROR,		"crypto error") \
	func(APKE_CRYPTO_NOT_SUPPORTED,	"cryptographic algorithm not supported") \
	func(APKE_CRYPTO_KEY_FORMAT,	"cryptographic key format not recognized") \
	func(APKE_SIGNATURE_GEN_FAILURE,"signing failure") \
	func(APKE_SIGNATURE_UNTRUSTED,	"UNTRUSTED signature") \
	func(APKE_SIGNATURE_INVALID,	"BAD signature") \
	func(APKE_FORMAT_INVALID,	"file format is invalid or inconsistent") \
	func(APKE_FORMAT_OBSOLETE,	"file format is obsolete (e.g. missing embedded checksum)") \
	func(APKE_FORMAT_NOT_SUPPORTED,	"file format not supported (in this applet)") \
	func(APKE_PKGNAME_FORMAT,	"package name is invalid") \
	func(APKE_PKGVERSION_FORMAT,	"package version is invalid") \
	func(APKE_DEPENDENCY_FORMAT,	"dependency format is invalid") \
	func(APKE_ADB_COMPRESSION,	"ADB compression not supported") \
	func(APKE_ADB_HEADER,		"ADB header error") \
	func(APKE_ADB_VERSION,		"incompatible ADB version") \
	func(APKE_ADB_SCHEMA,		"ADB schema error") \
	func(APKE_ADB_BLOCK,		"ADB block error") \
	func(APKE_ADB_SIGNATURE,	"ADB signature block error") \
	func(APKE_ADB_INTEGRITY,	"ADB integrity error") \
	func(APKE_ADB_NO_FROMSTRING,	"ADB schema error (no fromstring)") \
	func(APKE_ADB_LIMIT,		"ADB schema limit reached") \
	func(APKE_ADB_PACKAGE_FORMAT,	"ADB package format") \
	func(APKE_V2DB_FORMAT,		"v2 database format error") \
	func(APKE_V2PKG_FORMAT,		"v2 package format error") \
	func(APKE_V2PKG_INTEGRITY,	"v2 package integrity error") \
	func(APKE_V2NDX_FORMAT,		"v2 index format error") \
	func(APKE_PACKAGE_NOT_FOUND,	"could not find a repo which provides this package (check repositories file and run 'apk update')") \
	func(APKE_PACKAGE_NAME_SPEC,	"package name specification is invalid") \
	func(APKE_INDEX_STALE,		"package mentioned in index not found (try 'apk update')") \
	func(APKE_FILE_INTEGRITY,	"file integrity error") \
	func(APKE_CACHE_NOT_AVAILABLE,	"cache not available") \
	func(APKE_UVOL_NOT_AVAILABLE,	"uvol manager not available") \
	func(APKE_UVOL_ERROR,		"uvol error") \
	func(APKE_UVOL_ROOT,		"uvol not supported with --root") \
	func(APKE_REMOTE_IO,		"remote server returned error (try 'apk update')") \
	func(APKE_NOT_EXTRACTED,	"file not extracted") \

const char *apk_error_str(int error)
{
	static const struct error_literals {
#define ERRMSG_DEFINE(n, str) char errmsg_##n[sizeof(str)];
		DECLARE_ERRMSGS(ERRMSG_DEFINE)
	} errors = {
#define ERRMSG_ASSIGN(n, str) str,
		DECLARE_ERRMSGS(ERRMSG_ASSIGN)
	};
	static const unsigned short errmsg_index[] = {
#define ERRMSG_INDEX(n, str) [n - APKE_FIRST_VALUE] = offsetof(struct error_literals, errmsg_##n),
		DECLARE_ERRMSGS(ERRMSG_INDEX)
	};

	if (error < 0) error = -error;
	if (error >= APKE_FIRST_VALUE && error < APKE_FIRST_VALUE + ARRAY_SIZE(errmsg_index))
		return (char *)&errors + errmsg_index[error - APKE_FIRST_VALUE];
	return strerror(error);
}

int apk_exit_status_str(int status, char *buf, size_t sz)
{
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return 0;
	if (WIFEXITED(status))
		return apk_fmt(buf, sz, "exited with error %d", WEXITSTATUS(status));
	if (WIFSIGNALED(status))
		return apk_fmt(buf, sz, "killed by signal %d", WTERMSIG(status));
	if (WIFSTOPPED(status))
		return apk_fmt(buf, sz, "stopped by signal %d", WSTOPSIG(status));
	if (WIFCONTINUED(status))
		return apk_fmt(buf, sz, "continued");
	return apk_fmt(buf, sz, "status unknown %x", status);
}

static const char *size_units[] = {"B", "KiB", "MiB", "GiB", "TiB"};

int apk_get_human_size_unit(apk_blob_t b)
{
	for (int i = 0, s = 1; i < ARRAY_SIZE(size_units); i++, s *= 1024)
		if (apk_blob_compare(b, APK_BLOB_STR(size_units[i])) == 0)
			return s;
	return 1;
}

const char *apk_get_human_size(off_t size, off_t *dest)
{
	size_t i;
	off_t s;

	assert(size >= 0);

	for (i = 0, s = size; s >= 10000 && i < ARRAY_SIZE(size_units); i++)
		s /= 1024;

	if (dest) *dest = s;
	return size_units[min(i, ARRAY_SIZE(size_units) - 1)];
}

const char *apk_last_path_segment(const char *path)
{
	const char *last = strrchr(path, '/');
	return last == NULL ? path : last + 1;
}

void apk_url_parse(struct apk_url_print *urlp, const char *url)
{
	const char *authority, *path_or_host, *pw;

	*urlp = (struct apk_url_print) {
		.url = "",
		.pwmask = "",
		.url_or_host = url,
	};

	if (!(authority = strstr(url, "://"))) return;
	authority += 3;
	path_or_host = strpbrk(authority, "/@");
	if (!path_or_host || *path_or_host == '/') return;
	pw = strpbrk(authority, "@:");
	if (!pw || *pw == '@') return;
	*urlp = (struct apk_url_print) {
		.url = url,
		.pwmask = "*",
		.url_or_host = path_or_host,
		.len_before_pw = pw - url + 1,
	};
}

void apk_out_reset(struct apk_out *out)
{
	out->width = 0;
}

static int apk_out_get_width(struct apk_out *out)
{
	struct winsize w;

	if (out->width == 0) {
		out->width = 50;
		if (ioctl(fileno(out->out), TIOCGWINSZ, &w) == 0 &&
		    w.ws_col > 25)
			out->width = w.ws_col;
	}

	return out->width;
}

static void apk_out_render_progress(struct apk_out *out, bool force)
{
	struct apk_progress *p = out->prog;
	int i, bar_width, bar = 0, percent = 0;

	if (!p || out->progress_disable) return;
	if (out->width == 0) force = true;

	bar_width = apk_out_get_width(out) - 6;
	if (p->max_progress > 0) {
		bar = muldiv(bar_width, p->cur_progress, p->max_progress);
		percent = muldiv(100, p->cur_progress, p->max_progress);
	}
	if (force || bar != p->last_bar || percent != p->last_percent) {
		FILE *f = out->out;
		p->last_bar = bar;
		p->last_percent = percent;
		fprintf(f, "\e7%3i%% ", percent);
		for (i = 0; i < bar;  i++) fputs(p->out->progress_char, f);
		for (; i < bar_width; i++) fputc(' ', f);
		fflush(f);
		fputs("\e8\e[0K", f);
		out->need_flush = 1;
	}
}

static void log_internal(FILE *dest, const char *prefix, const char *format, va_list va)
{
	if (prefix != NULL && prefix != APK_OUT_LOG_ONLY && prefix[0] != 0) fputs(prefix, dest);
	vfprintf(dest, format, va);
	fputc('\n', dest);
	fflush(dest);
}

void apk_out_progress_note(struct apk_out *out, const char *format, ...)
{
	char buf[512];
	va_list va;
	int n, width = apk_out_get_width(out);
	FILE *f = out->out;

	if (out->progress_disable) return;

	va_start(va, format);
	n = vsnprintf(buf, sizeof buf, format, va);
	va_end(va);
	if (n >= width-4) strcpy(&buf[width-7], "...");
	fprintf(f, "\e7[%s]", buf);
	fflush(f);
	fputs("\e8\e[0K", f);
	out->need_flush = 1;
}

void apk_out_fmt(struct apk_out *out, const char *prefix, const char *format, ...)
{
	va_list va;
	if (prefix != APK_OUT_LOG_ONLY) {
		va_start(va, format);
		if (prefix && out->need_flush) fflush(out->out);
		log_internal(prefix ? out->err : out->out, prefix, format, va);
		out->need_flush = 0;
		va_end(va);
		apk_out_render_progress(out, true);
	}

	if (out->log) {
		va_start(va, format);
		log_internal(out->log, prefix, format, va);
		va_end(va);
	}
}

void apk_out_log_argv(struct apk_out *out, char **argv)
{
	char when[32];
	struct tm tm;
	time_t now = time(NULL);

	if (!out->log) return;
	fprintf(out->log, "\nRunning `");
	for (int i = 0; argv[i]; ++i) fprintf(out->log, "%s%s", argv[i], argv[i+1] ? " " : "");

	gmtime_r(&now, &tm);
	strftime(when, sizeof(when), "%Y-%m-%d %H:%M:%S", &tm);
	fprintf(out->log, "` at %s\n", when);
}

size_t apk_progress_weight(size_t bytes, size_t packages)
{
	return bytes + packages * 1024 * 64;
}

void apk_progress_start(struct apk_progress *p, struct apk_out *out, const char *stage, size_t max_progress)
{
	*p = (struct apk_progress) {
		.out = out,
		.stage = stage,
		.max_progress = max_progress,
		.item_base_progress = 0,
		.item_max_progress = max_progress,
	};
	out->prog = p;
}

void apk_progress_update(struct apk_progress *p, size_t cur_progress)
{
	if (cur_progress >= p->item_max_progress) cur_progress = p->item_max_progress;
	cur_progress += p->item_base_progress;

	if (cur_progress == p->cur_progress) return;

	int progress_fd = p->out->progress_fd;
	if (progress_fd != 0) {
		char buf[64]; /* enough for petabytes... */
		int i = apk_fmt(buf, sizeof buf, "%zu/%zu %s\n", cur_progress, p->max_progress, p->stage);
		if (i < 0 || apk_write_fully(progress_fd, buf, i) != i) {
			close(progress_fd);
			p->out->progress_fd = 0;
		}
	}
	p->cur_progress = cur_progress;
	apk_out_render_progress(p->out, false);
}

void apk_progress_end(struct apk_progress *p)
{
	apk_progress_update(p, p->max_progress);
	p->out->prog = NULL;
}

void apk_progress_item_start(struct apk_progress *p, size_t base_progress, size_t max_item_progress)
{
	p->item_base_progress = p->cur_progress;
	p->item_max_progress = max_item_progress;
	apk_progress_update(p, 0);
}

void apk_progress_item_end(struct apk_progress *p)
{
	apk_progress_update(p, p->item_max_progress);
	p->item_max_progress = p->max_progress;
	p->item_base_progress = 0;
}

static void progress_get_meta(struct apk_istream *is, struct apk_file_meta *meta)
{
	struct apk_progress_istream *pis = container_of(is, struct apk_progress_istream, is);
	return apk_istream_get_meta(pis->pis, meta);
}

static ssize_t progress_read(struct apk_istream *is, void *ptr, size_t size)
{
	struct apk_progress_istream *pis = container_of(is, struct apk_progress_istream, is);
	ssize_t max_read = 1024*1024;
	ssize_t r;

	apk_progress_update(pis->p, pis->done);
	r = pis->pis->ops->read(pis->pis, ptr, (size > max_read) ? max_read : size);
	if (r > 0) pis->done += r;
	return r;
}

static int progress_close(struct apk_istream *is)
{
	struct apk_progress_istream *pis = container_of(is, struct apk_progress_istream, is);
	return apk_istream_close(pis->pis);
}

static const struct apk_istream_ops progress_istream_ops = {
	.get_meta = progress_get_meta,
	.read = progress_read,
	.close = progress_close,
};

struct apk_istream *apk_progress_istream(struct apk_progress_istream *pis, struct apk_istream *is, struct apk_progress *p)
{
	if (IS_ERR(is) || !p) return is;
	*pis = (struct apk_progress_istream) {
		.is.ops = &progress_istream_ops,
		.is.buf = is->buf,
		.is.buf_size = is->buf_size,
		.is.ptr = is->ptr,
		.is.end = is->end,
		.pis = is,
		.p = p,
	};
	pis->done += (pis->is.end - pis->is.ptr);
	return &pis->is;
}

void apk_print_indented_init(struct apk_indent *i, struct apk_out *out, int err)
{
	*i = (struct apk_indent) {
		.f = err ? out->err : out->out,
		.width = apk_out_get_width(out),
	};
}

void apk_print_indented_line(struct apk_indent *i, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vfprintf(i->f, fmt, va);
	va_end(va);
	i->x = i->indent = 0;
}

void apk_print_indented_group(struct apk_indent *i, int indent, const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	i->x = vfprintf(i->f, fmt, va);
	i->indent = indent ?: (i->x + 1);
	if (fmt[strlen(fmt)-1] == '\n') i->x = 0;
	va_end(va);
}

void apk_print_indented_end(struct apk_indent *i)
{
	if (i->x) {
		fprintf(i->f, "\n");
		i->x = i->indent = 0;
	}
}

int apk_print_indented(struct apk_indent *i, apk_blob_t blob)
{
	if (i->x <= i->indent)
		i->x += fprintf(i->f, "%*s" BLOB_FMT, i->indent - i->x, "", BLOB_PRINTF(blob));
	else if (i->x + blob.len + 1 >= i->width)
		i->x = fprintf(i->f, "\n%*s" BLOB_FMT, i->indent, "", BLOB_PRINTF(blob)) - 1;
	else
		i->x += fprintf(i->f, " " BLOB_FMT, BLOB_PRINTF(blob));
	return 0;
}

void apk_print_indented_words(struct apk_indent *i, const char *text)
{
	apk_blob_foreach_word(word, APK_BLOB_STR(text))
		apk_print_indented(i, word);
}

void apk_print_indented_fmt(struct apk_indent *i, const char *fmt, ...)
{
	char tmp[256];
	size_t n;
	va_list va;

	va_start(va, fmt);
	n = vsnprintf(tmp, sizeof tmp, fmt, va);
	apk_print_indented(i, APK_BLOB_PTR_LEN(tmp, n));
	va_end(va);
}
