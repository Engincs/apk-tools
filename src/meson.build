crypto_backend = get_option('crypto_backend')
url_backend = get_option('url_backend')

libapk_so_version = so_version
libapk_src = [
	'adb.c',
	'adb_comp.c',
	'adb_walk_adb.c',
	'apk_adb.c',
	'atom.c',
	'balloc.c',
	'blob.c',
	'commit.c',
	'common.c',
	'context.c',
	'crypto.c',
	'crypto_@0@.c'.format(crypto_backend),
	'ctype.c',
	'database.c',
	'extract_v2.c',
	'extract_v3.c',
	'fs_fsys.c',
	'fs_uvol.c',
	'hash.c',
	'io.c',
	'io_gunzip.c',
	'io_url_@0@.c'.format(url_backend),
	'package.c',
	'pathbuilder.c',
	'print.c',
	'process.c',
	'query.c',
	'repoparser.c',
	'serialize.c',
	'serialize_json.c',
	'serialize_yaml.c',
	'solver.c',
	'tar.c',
	'trust.c',
	'version.c',
]

libapk_headers = [
	'adb.h',
	'apk_atom.h',
	'apk_balloc.h',
	'apk_blob.h',
	'apk_context.h',
	'apk_crypto.h',
	'apk_ctype.h',
	'apk_database.h',
	'apk_defines.h',
	'apk_extract.h',
	'apk_fs.h',
	'apk_hash.h',
	'apk_io.h',
	'apk_package.h',
	'apk_pathbuilder.h',
	'apk_print.h',
	'apk_provider_data.h',
	'apk_query.h',
	'apk_repoparser.h',
	'apk_serialize.h',
	'apk_solver_data.h',
	'apk_solver.h',
	'apk_tar.h',
	'apk_trust.h',
	'apk_version.h',
]

apk_src = [
	'apk.c',
	'app_adbdump.c',
	'app_adbgen.c',
	'app_adbsign.c',
	'app_add.c',
	'app_audit.c',
	'app_cache.c',
	'app_convdb.c',
	'app_convndx.c',
	'app_del.c',
	'app_dot.c',
	'app_extract.c',
	'app_fetch.c',
	'app_fix.c',
	'app_index.c',
	'app_info.c',
	'app_list.c',
	'app_manifest.c',
	'app_mkndx.c',
	'app_mkpkg.c',
	'app_policy.c',
	'app_query.c',
	'app_update.c',
	'app_upgrade.c',
	'app_search.c',
	'app_stats.c',
	'app_verify.c',
	'app_version.c',
	'applet.c',
	'serialize_query.c',
]

apk_cargs = [
	'-D_ATFILE_SOURCE',
	'-DAPK_VERSION="' + meson.project_version() + '"',
]

if lua_bin.found()
	genhelp_script = files('genhelp.lua')
	genhelp_args = [lua_bin, genhelp_script, '@INPUT@']

	if not get_option('compressed-help')
		genhelp_args += ['--no-zlib']
	endif

	generated_help = custom_target(
		'help.h',
		capture: true,
		output: 'help.h',
		input: man_files,
		command: genhelp_args,
	)
else
	generated_help = custom_target(
		'help.h',
		capture: true,
		output: 'help.h',
		command: ['echo', '#define NO_HELP'],
	)
endif

apk_src += [ generated_help ]

apk_arch = get_option('arch')
if apk_arch != ''
	apk_cargs += ['-DAPK_CONFIG_ARCH="@0@"'.format(apk_arch)]
else
	apk_arch_prefix = get_option('arch_prefix')
	if apk_arch_prefix != ''
		apk_cargs += ['-DAPK_CONFIG_ARCH_PREFIX="@0@"'.format(apk_arch_prefix)]
	endif
endif

apk_uvol_db_target = get_option('uvol_db_target').strip('/')
if apk_uvol_db_target != ''
	apk_cargs += ['-DAPK_UVOL_DB_TARGET="@0@"'.format(apk_uvol_db_target)]
endif

if get_option('zstd')
	libapk_src += [ 'io_zstd.c' ]
	apk_cargs += [ '-DHAVE_ZSTD' ]
endif

libapk_lib = library(
	'apk',
	libapk_src,
	soversion: libapk_so_version,
	install: not subproject,
	dependencies: [
		libfetch_dep,
		libportability_dep,
		apk_deps,
	],
	c_args: apk_cargs,
)

libapk_dep = declare_dependency(
	link_with: libapk_lib,
	include_directories: ['.'],
)

if not subproject
	pkgc.generate(
		libapk_lib,
		name: 'apk',
		version: libapk_so_version,
	)

	install_headers(
		libapk_headers,
		subdir: 'apk',
	)
endif

apk_exe = executable(
	'apk',
	apk_src,
	install: not subproject,
	dependencies: [
		libapk_dep,
		apk_deps,
		libfetch_dep.partial_dependency(includes: true),
		libportability_dep.partial_dependency(includes: true),
	],
	c_args: apk_cargs,
)
