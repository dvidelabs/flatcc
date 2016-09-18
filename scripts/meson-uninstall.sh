#!/bin/sh

DESTDIR=${DESTDIR:-/usr/local}
RM=false
RMALL=false
RMDIR=false

echo "DESTDIR=$DESTDIR"

case "$1" in
    dry-run=yes)
        echo "dry-run, listing files:"
        RM="ls -l"
        RMALL="ls -l"
        RMDIR="ls -l"
        ;;
    confirm-uninstall=yes)
        RM=rm
        RMALL="rm -rf"
        RMDIR=rmdir
        ;;
    *)
        echo "This script is a an example of how one might uninstall flatcc"
        echo "This script is not designed for your system"
        echo "If you insist, use at own script"
        echo "set DESTDIR variable accordingly"
        echo "then call again with $(basename $0) dry-run=yes"
        echo "then call again with $(basename $0) confirm-uninstall=yes"
        exit -1
esac


found=no
if [ -f $DESTDIR/bin/flatcc ]; then
    $RM $DESTDIR/bin/flatcc
    found=yes
fi

#if [ -e "$DESTDIR/lib/libflatcc*" ]; then
for f in $DESTDIR/include/flatcc/*; do
    if [ -e "$f" ]; then
        $RMALL $DESTDIR/lib/libflatcc*
        found=yes
        break;
    fi
done

if [ -d $DESTDIR/include/flatcc ]; then
    for f in $DESTDIR/include/flatcc/*; do
        if [ -e "$f" ]; then
            $RMALL $DESTDIR/include/flatcc/*
            found=yes
            break
        fi
    done
    $RMDIR $DESTDIR/include/flatcc
    found=yes
fi

if [ "$found" == "no" ]; then
    echo "no installed files detected"
else
    echo "files uninstalled"
fi

