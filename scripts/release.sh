#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

usage()
{
    cat <<EOF
Usage:
    $(basename "$0") RELEASE_VERSION NEXT_VERSION

Example:
    $(basename "$0") 0.6.3 0.6.4-pre

This script will:

  1. Refuse to run if the git tree is dirty.
  2. Set the release version.
  3. Build flatcc.
  4. Verify the compiler reports the release version.
  5. Regenerate checked-in reflection sources.
  6. Run the full test suite.
  7. Commit and tag the release.
  8. Bump to the next development version.
  9. Rebuild flatcc.
 10. Regenerate reflection sources.
 11. Commit the development version.

Nothing is pushed to the remote repository.
EOF
    exit 1
}

[ $# -eq 2 ] || usage

RELEASE="$1"
NEXT="$2"

if [ -n "$(git status --porcelain)" ]; then
    echo "Working tree is not clean."
    echo "Commit or stash changes before releasing."
    exit 1
fi

echo "== Preparing release $RELEASE =="

scripts/update-version.sh "$RELEASE"

scripts/build.sh

echo "Checking compiler version..."
bin/flatcc --version | grep -F "$RELEASE" >/dev/null

scripts/generate.sh

scripts/test.sh

git add \
    include/flatcc/flatcc_version.h \
    include/flatcc/reflection

git commit -m "Release $RELEASE"

git tag -a "v$RELEASE" -m "flatcc $RELEASE"

echo
echo "== Starting development of $NEXT =="

scripts/update-version.sh "$NEXT"

scripts/build.sh

echo "Checking compiler version..."
bin/flatcc --version | grep -F "$NEXT" >/dev/null

git add \
    include/flatcc/flatcc_version.h \
    include/flatcc/reflection

git commit -m "Start development $NEXT"

echo
echo "Release complete."
echo
echo "Created tag:"
echo "    v$RELEASE"
echo
echo "Repository is now at:"
echo "    $NEXT"
echo
echo "Don't forget:"
echo "    git push origin main --follow-tags"
