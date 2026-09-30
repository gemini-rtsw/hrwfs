#!/bin/bash
# Check a finished build before it ships. The spec runs this after `make`, and
# a developer can run it after a local `make` -- same checks, same values, so a
# build that passes here passes in the pipeline. Every check below has failed
# for real at least once; each one is silent at build time and fatal at boot.
#
#   ./tools/linux-build/check-build.sh
set -u
TOP="$(cd "$(dirname "$0")/../.." && pwd -P)"
cd "$TOP"
. tools/linux-build/build.conf
SUP=/gemini/epics3.13.4/support
rc=0
fail() { echo "ERROR: $*" >&2; rc=1; }

# macTest leaves a macro it does not know as literal text.
grep -rlI -e '$(slalib_ver)' -e '$(timelib_ver)' -e '$(astlib_ver)' -e '$(cfitsio_ver)' bin 2>/dev/null \
    | sed 's/^/  /' | grep . >&2 && fail "unsubstituted version macros remain (above)"

for v in "$SLALIB_VER" "$TIMELIB_VER" "$ASTLIB_VER" "$CFITSIO_VER"; do
    grep -q "$SUP/[a-z]*/$v/" bin/ppc604/startup || fail "bin/ppc604/startup does not reference $v"
done

for f in bin/ppc604/startup bin/ppc604/local; do
    grep -q "cd \"$DEPLOY\"" "$f" 2>/dev/null || fail "$f does not cd to $DEPLOY: $(grep '^cd ' "$f" 2>/dev/null)"
done

# The build directory must not leak into anything that ships.
grep -rlI -e '/root/rpmbuild' -e "$TOP" bin 2>/dev/null | sed 's/^/  /' | grep . >&2 \
    && fail "the build directory appears in files that will ship (above)"

for f in wfsLibrariesHrwfs wfsHrwfsDb detControl seqControl autoPath \
         simpleLogHrwfs wfsResourceMonitor hrwfsConfig wfsSite fpscr gemini.Support; do
    [ -f "bin/ppc604/$f" ] || fail "bin/ppc604/$f was not built"
done
for f in data/hrwfsTop.db data/hrwfsSadTop.db dbd/gemini.dbd; do
    [ -f "$f" ] || fail "$f missing"
done

[ $rc -eq 0 ] && echo "check-build: OK"
exit $rc
