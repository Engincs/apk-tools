##
# Building apk-tools

-include config.mk

PACKAGE := apk-tools
VERSION := 2.6.10

##
# Default directories

DESTDIR		:=
SBINDIR		:= /sbin
LIBDIR		:= /lib
CONFDIR		:= /etc/apk
MANDIR		:= /usr/share/man
DOCDIR		:= /usr/share/doc/apk

export DESTDIR SBINDIR LIBDIR CONFDIR MANDIR DOCDIR

##
# Top-level rules and targets

targets		:= src/

##
# Include all rules and stuff

include Make.rules

##
# Top-level targets

install:
	$(INSTALLDIR) $(DESTDIR)$(DOCDIR)
	$(INSTALL) README $(DESTDIR)$(DOCDIR)

test: FORCE
	$(Q)$(MAKE) TEST=y
	$(Q)$(MAKE) -C test

static:
	$(Q)$(MAKE) STATIC=y
