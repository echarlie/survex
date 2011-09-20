#!/bin/sh
#
# Survex test suite - compare 2 versions of cavern on a dataset
# Copyright (C) 1999-2003 Olly Betts
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

testdir=`echo $0 | sed 's!/[^/]*$!!' || echo '.'`

# allow us to run tests standalone more easily
: ${srcdir="$testdir"}

# force VERBOSE if we're run on a subset of tests
#test -n "$*" && VERBOSE=1

test -x "$testdir"/../src/cavern || testdir=.

: ${CAVERN="$testdir"/../src/cavern}
: ${CAVERN_ORIG=cavern}
: ${DIFFPOS="$testdir"/../src/diffpos}

: ${TESTS=${*-""}}

for file in $TESTS ; do
  if test -n "$file" ; then
    echo "$file"
    rm -f tmp.* tmp_orig.*
    if test -n "$VERBOSE" ; then
      $CAVERN_ORIG "$file" --output=tmp_orig | tee tmp_orig.out || exit 1
      $CAVERN "$file" --output=tmp | tee tmp.out || exit 1
    else
      $CAVERN_ORIG "$file" --output=tmp_orig > tmp_orig.out || exit 1
      $CAVERN "$file" --output=tmp > tmp.out || exit 1
    fi
    warn_orig=`sed '$!d;$s/^Done.*/0/;s/[^0-9]*\([0-9]*\).*/\1/' tmp_orig.out`
    warn=`sed '$!d;$s/^Done.*/0/;s/[^0-9]*\([0-9]*\).*/\1/' tmp.out`
    if test x"$warn_orig" != x"$warn" ; then
      echo "$CAVERN_ORIG gave $warn_orig warning(s)"
      echo "$CAVERN gave $warn warning(s)"
      exit 1
    fi
    if test -n "$VERBOSE" ; then
      $DIFFPOS tmp.3d tmp_orig.3d || exit 1
    else
      $DIFFPOS tmp.3d tmp_orig.3d > /dev/null || exit 1
    fi
  fi
done
test -n "$VERBOSE" && echo "Test passed"
exit 0
