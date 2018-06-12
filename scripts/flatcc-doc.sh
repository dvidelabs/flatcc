#!/bin/sh

HOME=$(dirname $0)/..
SCHEMA=${SCHEMA:-$1}
PREFIX=${PREFIX:-$2}
OUTDIR=${OUTDIR:-$3}
OUTDIR=${OUTDIR:-'.'}
INCLUDE=${INLCUDE:-$HOME/include}
FLATCC=${FLATCC:-$HOME/bin/flatcc}

if [ "x$SCHEMA" = "x" ]; then
    echo "Missing schema arg"
    echo "usage: $(basename $0) schema-file name-prefix [outdir]"
    exit 1
fi

if [ "x$PREFIX" = "x" ]; then
    echo "Missing prefix arg"
    echo "usage: $(basename $0) schema-file name-prefix [outdir]"
    exit 1
fi

echo "generating doc for schema: '$SCHEMA' with name prefix: '$PREFIX' to '$OUTDIR'"

echo generating $OUTDIR/$PREFIX.reader.doc
$FLATCC $SCHEMA -cr --reader --stdout | \
    clang - -E -DNDEBUG -I $INCLUDE | \
    clang-format -style="WebKit" | \
    grep "^static .* $PREFIX" | \
    cut -f 1 -d '{' | \
    grep -v deprecated \
    > $OUTDIR/$PREFIX.reader.doc

echo generating $OUTDIR/$PREFIX.builder.doc
$FLATCC $SCHEMA -cr --builder --stdout | \
    clang - -E -DNDEBUG -I $INCLUDE | \
    clang-format -style="WebKit" | \
    grep "^static .* $PREFIX" | \
    cut -f 1 -d '{' | \
    grep -v deprecated \
    > $OUTDIR/$PREFIX.builder.doc

echo generating $OUTDIR/$PREFIX.verifier.doc
$FLATCC $SCHEMA -cr --verifier --stdout | \
    clang - -E -DNDEBUG -I $INCLUDE | \
    clang-format -style="WebKit" | \
    grep "^static .* $PREFIX" | \
    cut -f 1 -d '{' | \
    grep -v deprecated \
    > $OUTDIR/$PREFIX.verifier.doc
