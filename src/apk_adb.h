#include "adb.h"

/* Schemas */
#define ADB_SCHEMA_INDEX	0x78646e69	// indx
#define ADB_SCHEMA_PACKAGE	0x676b6370	// pckg
#define ADB_SCHEMA_INSTALLED_DB	0x00626469	// idb

/* Dependency */
#define ADBI_DEP_NAME		0x01
#define ADBI_DEP_VERSION	0x02
#define ADBI_DEP_MATCH		0x03
#define ADBI_DEP_MAX		0x04

/* Package Info */
#define ADBI_PI_NAME		0x01
#define ADBI_PI_VERSION		0x02
#define ADBI_PI_UNIQUE_ID	0x03
#define ADBI_PI_DESCRIPTION	0x04
#define ADBI_PI_ARCH		0x05
#define ADBI_PI_LICENSE		0x06
#define ADBI_PI_ORIGIN		0x07
#define ADBI_PI_MAINTAINER	0x08
#define ADBI_PI_URL		0x09
#define ADBI_PI_REPO_COMMIT	0x0a
#define ADBI_PI_BUILD_TIME	0x0b
#define ADBI_PI_INSTALLED_SIZE	0x0c
#define ADBI_PI_FILE_SIZE	0x0d
#define ADBI_PI_PRIORITY	0x0e
#define ADBI_PI_DEPENDS		0x0f
#define ADBI_PI_PROVIDES	0x10
#define ADBI_PI_REPLACES	0x11
#define ADBI_PI_INSTALL_IF	0x12
#define ADBI_PI_RECOMMENDS	0x13
#define ADBI_PI_MAX		0x14

/* File Info */
#define ADBI_FI_NAME		0x01
#define ADBI_FI_HASHES		0x02
#define ADBI_FI_FILES		0x02
#define ADBI_FI_MODE		0x03
#define ADBI_FI_UID		0x04
#define ADBI_FI_GID		0x05
#define ADBI_FI_XATTRS		0x06
#define ADBI_FI_MAX		0x07

/* Scripts */
#define ADBI_SCRPT_TRIGGER	0x01
#define ADBI_SCRPT_PREINST	0x02
#define ADBI_SCRPT_POSTINST	0x03
#define ADBI_SCRPT_PREDEINST	0x04
#define ADBI_SCRPT_POSTDEINST	0x05
#define ADBI_SCRPT_PREUPGRADE	0x06
#define ADBI_SCRPT_POSTUPGRADE	0x07
#define ADBI_SCRPT_MAX		0x08

/* Package */
#define ADBI_PKG_PKGINFO	0x01
#define ADBI_PKG_PATHS		0x02
#define ADBI_PKG_SCRIPTS	0x03
#define ADBI_PKG_TRIGGERS	0x04
#define ADBI_PKG_PASSWD		0x05
#define ADBI_PKG_MAX		0x06

/* Index */
#define ADBI_NDX_DESCRIPTION	0x01
#define ADBI_NDX_PACKAGES	0x02
#define ADBI_NDX_MAX		0x03

/* Installed DB */
#define ADBI_IDB_PACKAGES	0x01
#define ADBI_IDB_MAX		0x02

/* */
#define APK_MAX_PKG_DEPENDENCIES	512
#define APK_MAX_PKG_REPLACES		32
#define APK_MAX_PKG_TRIGGERS		32
#define APK_MAX_INDEX_PACKAGES		20000
#define APK_MAX_MANIFEST_FILES		8000
#define APK_MAX_MANIFEST_PATHS		6000

extern const struct adb_object_schema
	schema_dependency, schema_dependency_array,
	schema_pkginfo, schema_pkginfo_array,
	schema_file, schema_file_array, schema_path, schema_path_array,
	schema_string_array, schema_scripts, schema_package, schema_package_adb_array,
	schema_index, schema_idb;

/* */
int apk_dep_split(apk_blob_t *b, apk_blob_t *bdep);
adb_val_t adb_wo_pkginfo(struct adb_obj *obj, unsigned int f, apk_blob_t val);
unsigned int adb_pkg_field_index(char f);