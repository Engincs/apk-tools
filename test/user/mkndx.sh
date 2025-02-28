#!/bin/sh

# shellcheck disable=SC2016 # no expansion for pkgname-spec

TESTDIR=$(realpath "${TESTDIR:-"$(dirname "$0")"/..}")
. "$TESTDIR"/testlib.sh

setup_apkroot
APK="$APK --allow-untrusted --no-interactive --no-cache"

$APK mkpkg -I name:test-a -I version:1.0 -o test-a-1.0.apk
$APK mkpkg -I name:test-b -I version:1.0 -o test-b-1.0.apk

$APK mkndx -q -o index.adb test-a-1.0.apk
$APK mkndx -vv -o index-reindex.adb -x index.adb test-a-1.0.apk test-b-1.0.apk | diff -u /dev/fd/4 4<<EOF - || assert "wrong mkndx result"
test-a-1.0.apk: indexed from old index
test-b-1.0.apk: indexed new package
Index has 2 packages (of which 1 are new)
EOF

$APK mkndx --pkgname-spec 'https://test/${name}-${version}.apk' -o index.adb test-a-1.0.apk test-b-1.0.apk
$APK fetch --url --simulate --from none --repository index.adb --pkgname-spec '${name}_${version}.pkg' test-a test-b 2>&1 | diff -u /dev/fd/4 4<<EOF - || assert "wrong fetch result"
https://test/test-a-1.0.apk
https://test/test-b-1.0.apk
EOF

$APK mkndx --pkgname-spec '${name:3}/${name}-${version}.apk' -o index.adb test-a-1.0.apk test-b-1.0.apk
$APK fetch --url --simulate --from none --repository "test:/$PWD/index.adb" --pkgname-spec '${name}_${version}.pkg' test-a test-b 2>&1 | diff -u /dev/fd/4 4<<EOF - || assert "wrong fetch result"
test:/$PWD/tes/test-a-1.0.apk
test:/$PWD/tes/test-b-1.0.apk
EOF

$APK mkndx --pkgname-spec '${name:3}/${name}-${version}.apk' -o index.adb test-a-1.0.apk test-b-1.0.apk
$APK fetch --url --simulate --from none --repository index.adb --pkgname-spec '${name}_${version}.pkg' test-a test-b 2>&1 | diff -u /dev/fd/4 4<<EOF - || assert "wrong fetch result"
./tes/test-a-1.0.apk
./tes/test-b-1.0.apk
EOF

$APK mkndx -vv --filter-spec '${name}-${version}' --pkgname-spec 'http://test/${name}-${version}.apk' -x index.adb -o index-filtered.adb test-a-1.0
$APK fetch --url --simulate --from none --repository index-filtered.adb --pkgname-spec '${name}_${version}.pkg' test-a 2>&1 | diff -u /dev/fd/4 4<<EOF - || assert "wrong fetch result"
http://test/test-a-1.0.apk
EOF
