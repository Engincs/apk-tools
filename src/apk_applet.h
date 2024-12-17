/* apk_applet.h - Alpine Package Keeper (APK)
 *
 * Copyright (C) 2005-2008 Natanael Copa <n@tanael.org>
 * Copyright (C) 2008-2011 Timo Teräs <timo.teras@iki.fi>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef APK_APPLET_H
#define APK_APPLET_H

#include <errno.h>
#include <getopt.h>
#include "apk_defines.h"
#include "apk_database.h"

#define __APK_OPT_ENUM(_enum,__desc) _enum,
#define __APK_OPT_DESC(_enum,__desc) __desc "\x00"

#define APK_OPT_ARG		"\xaf"
#define APK_OPT_SH(x)		"\xf1" x
#define APK_OPT_S2(x)		"\xf2" x

#define APK_OPTIONS(var_name, init_macro) \
	enum { init_macro(__APK_OPT_ENUM) }; \
	static const char var_name[] = init_macro(__APK_OPT_DESC);

#define APK_OPTIONS_INIT 0xffff00

struct apk_applet {
	struct list_head node;

	const char *name;
	const char *options_desc;

	unsigned int optgroup_commit : 1;
	unsigned int optgroup_generation : 1;
	unsigned int optgroup_source : 1;
	unsigned int remove_empty_arguments : 1;
	unsigned int open_flags, forced_force;
	int context_size;

	int (*parse)(void *ctx, struct apk_ctx *ac, int opt, const char *optarg);
	int (*main)(void *ctx, struct apk_ctx *ac, struct apk_string_array *args);
};

void apk_applet_register(struct apk_applet *);
struct apk_applet *apk_applet_find(const char *name);
void apk_applet_help(struct apk_applet *applet, struct apk_out *out);

#define APK_DEFINE_APPLET(x) \
__attribute__((constructor)) static void __register_##x(void) { apk_applet_register(&x); }

#endif
