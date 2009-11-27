#!/bin/sh

if test -d ".svn"; then

svn_revision=`cd "$1" && LC_ALL=C svn info 2> /dev/null | grep Revision | cut -d' ' -f2`
test $svn_revision || svn_revision=`cd "$1" && grep revision .svn/entries 2>/dev/null | cut -d '"' -f2`
test $svn_revision || svn_revision=`cd "$1" && sed -n -e '/^dir$/{n;p;q}' .svn/entries 2>/dev/null`
test $svn_revision || svn_revision=UNKNOWN

NEW_REVISION="#define FUPPES_VERSION \"0.$svn_revision\""
OLD_REVISION=`cat version.h 2> /dev/null`

# Update version.h only on revision changes to avoid spurious rebuilds
if test "$NEW_REVISION" != "$OLD_REVISION"; then
    echo "$NEW_REVISION" > version.h
fi

else

# Git have no logical revision structure so tag releases
revision=`git describe 2> /dev/null`
test $revision || revision="GIT-DEV"

NEW_REVISION="#define FUPPES_VERSION \"$revision\""
OLD_REVISION=`cat version.h 2> /dev/null`

# Update version.h only on revision changes to avoid spurious rebuilds
if test "$NEW_REVISION" != "$OLD_REVISION"; then
    echo "$NEW_REVISION" > version.h
fi

fi
