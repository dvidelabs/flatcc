#!/usr/bin/env bash

# also update FLATCC_VERSION_TEXT in config/config.h
# and if relevant regenerate reflection code using
# reflection/generated_code.sh
# followed by a test. This also updates the release number in
# headers based on config/config.h.
# and finally, remember to tag git, e.g.: git tag v0.0.1

VER=0.2.0

echo "archiving tagged version ${VER} to release folder"

cd `dirname $0`/..
mkdir -p release
git archive --format=tar.gz --prefix=flatcc-$VER/ v$VER >release/flatcc-$VER.tar.gz
