#!/bin/sh
set -u

# Copyright (C) 2017, Benjamin Drung <benjamin.drung@profitbricks.com>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

. "${0%/*}/shunit2-helper-functions.sh"

setUp() {
	create_repo
}

test_empty() {
	$REPREPRO -b $REPO export
	call $REPREPRO -b $REPO list buster
	assertEquals "" "$($REPREPRO -b $REPO list buster)"
}

test_list() {
	(cd $PKGS && PACKAGE=hello SECTION=main DISTRI=buster VERSION=1.0 REVISION=-1 ../genpackage.sh)
	call $REPREPRO -b $REPO -V -C main includedeb buster $PKGS/hello_1.0-1_${ARCH}.deb
	assertEquals "buster|main|$ARCH: hello 1.0-1" "$($REPREPRO -b $REPO list buster)"
}

test_ls() {
	(cd $PKGS && PACKAGE=hello SECTION=main DISTRI=buster EPOCH="1:" VERSION=2.5 REVISION=-3 ../genpackage.sh)
	call $REPREPRO -b $REPO -V -C main includedeb buster $PKGS/hello_2.5-3_${ARCH}.deb
	assertEquals "hello | 1:2.5-3 | buster | $ARCH" "$($REPREPRO -b $REPO ls hello)"
}

test_include_changes() {
	(cd $PKGS && PACKAGE=sl SECTION=main DISTRI=buster EPOCH="" VERSION=3.03 REVISION=-1 ../genpackage.sh)
	call $REPREPRO -b $REPO -V -C main include buster $PKGS/test.changes
	assertEquals "\
buster|main|amd64: sl 3.03-1
buster|main|amd64: sl-addons 3.03-1
buster|main|source: sl 3.03-1" "$($REPREPRO -b $REPO list buster)"
}

. shunit2