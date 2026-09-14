#!/bin/bash
# Build the hrwfs dependency RPMs from staged trees. These are the packages
# that do NOT build from source -- they copy prepared trees -- so they have no
# repo of their own and are not in the pipeline, the same arrangement gmoscc
# uses. Output: rpm/out/.
#
# Publish with gemini-rtsw-repo/upload-rpm.sh, then run the rebuild-latest
# workflow so they land in the served rpm-repo:latest.
#
# Each spec needs its own subtree staged under $HRWFS_BUILDENV/tree. A spec
# whose tree is absent is skipped with a message rather than failing the run,
# because the trees come from different hosts and are rarely all present:
#
#   usr/software/dev/packages/vxworks/tornado2.0  gem-tornado20-linux
#   usr/software/dev/packages/epics/epics3.13.4GEM7  gem-epics3134gem7
#   gemini/external/GEM7                          gem7-epics-runtime
#   gemini/dhs/dhs                                hrwfs-dhs-vxlibs
#   gemini/external/vxWorks/tornado2.0            gem-vxworks-tornado20
#
# gem-epics3134gem7 also needs the HOST TOOLS built into the staged tree
# first -- run tools/linux-build/build-host-tools.sh against it, or the spec's
# own check fails on base/bin/Linux/dbExpand.
set -e

HERE="$(cd "$(dirname "$0")" && pwd)"
BUILDENV="${HRWFS_BUILDENV:-$HOME/work/hrwfs-buildenv}"
TREES="$BUILDENV/tree"
OUT="$HERE/out"
mkdir -p "$OUT"

[ -d "$TREES" ] || { echo "ERROR: no staged trees at $TREES" >&2; exit 1; }

# spec:required-subtree
CANDIDATES="
gem-tornado20-linux:usr/software/dev/packages/vxworks/tornado2.0
gem-epics3134gem7:usr/software/dev/packages/epics/epics3.13.4GEM7
gem7-epics-runtime:gemini/external/GEM7
hrwfs-dhs-vxlibs:gemini/dhs/dhs
gem-vxworks-tornado20:gemini/external/vxWorks/tornado2.0
"
SPECS=""
for entry in $CANDIDATES; do
    spec="${entry%%:*}"; need="${entry##*:}"
    if [ ! -f "$HERE/$spec.spec" ]; then
        echo "skipping $spec - no spec yet"
    elif [ -e "$TREES/$need" ]; then
        SPECS="$SPECS $spec"
    else
        echo "skipping $spec - $TREES/$need not staged"
    fi
done
[ -n "$SPECS" ] || { echo "ERROR: nothing to build" >&2; exit 1; }
echo "building:$SPECS"

# The specs reference %{trees}/patches/... for files that live beside them.
mkdir -p "$HERE/staged-patches"
cp -f "$HERE/../patches/"* "$HERE/staged-patches/" 2>/dev/null || :

docker run --rm \
    -v "$TREES:/trees:ro" \
    -v "$HERE/staged-patches:/patches:ro" \
    -v "$HERE:/specs:ro" \
    -v "$OUT:/out" \
    -e SPECS="$SPECS" \
    rockylinux:9 bash -c '
        set -e
        dnf install -y -q rpm-build >/dev/null 2>&1
        # The specs read patches from %{trees}/patches; bind it in as a
        # sibling by pointing the macro at a directory that has both.
        mkdir -p /work && ln -sfn /trees/usr /work/usr && ln -sfn /trees/gemini /work/gemini \
            && ln -sfn /patches /work/patches
        for s in $SPECS; do
            echo "==== building $s"
            rpmbuild -bb --define "trees /work" --define "_rpmdir /out" /specs/$s.spec
        done
        ls -l /out/*/ 2>/dev/null || true
    '
echo
echo "RPMs in $OUT/:"
find "$OUT" -name '*.rpm' -printf '  %f\n' 2>/dev/null
