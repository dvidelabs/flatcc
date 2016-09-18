#!/bin/sh

PREFIX=${PREFIX:-/usr/local}

RM=false
RMALL=false
RMDIR=false

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
        echo "If you insist, be careful and set PREFIX suitably,"
        echo "it is not the same as DESTDIR if used during install."
        echo
        echo "test with $(basename $0) dry-run=yes"
        echo "then execute with $(basename $0) confirm-uninstall=yes"
        exit -1
esac

echo starting flatcc uninstall in $PREFIX

found=no

if [ -f $PREFIX/bin/flatcc ]; then
    $RM $PREFIX/bin/flatcc
    found=yes
fi

for f in $PREFIX/include/flatcc/*; do
    if [ -e "$f" ]; then
        $RMALL $PREFIX/lib/libflatcc*
        found=yes
        break;
    fi
done

if [ -d $PREFIX/include/flatcc ]; then
    for f in $PREFIX/include/flatcc/*; do
        if [ -e "$f" ]; then
            $RMALL $PREFIX/include/flatcc/*
            found=yes
            break
        fi
    done
    $RMDIR $PREFIX/include/flatcc
    found=yes
fi

if [ "$found" == "no" ]; then
    echo "no installed files detected in $PREFIX"
else
    echo "files uninstalled from $PREFIX"
fi

