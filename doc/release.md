# Release procedure

The follow steps describe a standard FlatCC release procedure.

The repository should be left clean after each step. Any modifications under
include/flatcc/reflection after running scripts/generate.sh should normally be
committed as part of the release.

A release can optionally be created in a tempory branch that is then merged after confirmation. Delete the branch after use to avoid future conflicts.

## Pre-release - prepare documentation:

1. Make sure CI and CI weekly builds are running and that local repo reflects
   online repository state without local changes in or out of tree.
2. Update `CHANGELOG.md` by removing '-pre' from current version and add empty next pre version early in the document.
3. Update Status section of README.md to reflect released version number, text should already
   have been updated to record important changes.

## Release - bump version, generate code, commit and tag:

1. Bump include/flatcc/flatcc_version.h from pre-release to release (use scripts/update-version.h)
   e.g. `scripts/version.sh 0.6.2` if current version is `0.6.2-pre`.
2. Build source with `scripts/build.sh` in order to update tool version.
3. Generate reflection code for bfbs schema files in reflection/reflection.fbs -> include/flatcc/reflection
   (use scripts/generate.sh). The generated headers include version in comment, so flatcc must be rebuild
   after version change as per above.
4. Run scripts/test.sh locally to verify things work as intended.
5. Commit source
6. Tag new version with v prefix, e.g. tag v0.6.2
7. Bump version to -pre release, e.g. scripts/version.sh 0.6.3-pre
8. Run test again (which rebuilds flatcc with updated version) to verify
   post-release (`-pre`) development.
9. Commit source without tagging.

The `scripts/release.sh` automates the above steps:

```sh
scripts/release.sh RELEASE_VERSION NEXT_VERSION
```

## Post release:

1. Push to main repository and observe CI builds, force weekly build.
2. When the release passes CI builds, create a release record in the hosted
   repository, copying CHANGELOG.md entries and/or Status notes from README
   as relevant along with pertinent information.


## Minor / Major versions

When preparing a new minor or major release, first stabilise and release the current maintenance branch as a point release if appropriate. Only then begin the new minor or major release cycle, after the point release has passed the weekly CI builds.

## Generated header comments

Generated headers should normally only be committed with released version numbers. If incompatible reflection changes prevent bootstrapping, it may temporarily be necessary to regenerate headers from a -pre compiler. In that case, disable FLATCC_REFLECTION, build flatcc without reflection support, regenerate the reflection headers, then re-enable FLATCC_REFLECTION. This breaks the bootstrap cycle and should only be needed when changing the reflection schema itself or
related code generation interface.
