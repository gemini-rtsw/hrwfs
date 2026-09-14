#!/bin/bash
# Build the EPICS 3.13.4GEM7 host tools for HOST_ARCH=Linux.
#
# These are NATIVE Linux binaries built with the system gcc -- no cross
# compiler involved. They are what the UAE build needs before it can do
# anything: dbExpand (expands the dbd), macTest (generates the vxWorks startup
# scripts from *.vws), snc (sequencer), antelope/e_flex (the yacc/lex the
# others are built from), dbToRecordtypeH, sf2db.
#
# Runs inside hrwfs-build:el9 with the writable tree bind-mounted at
# /usr/software -- the config files carry absolute paths, so the tree has to
# live where Solaris had it.
set -u

export EPICS=/usr/software/dev/packages/epics/epics3.13.4GEM7
export EPICS_BASE=$EPICS/base
export HOST_ARCH=Linux
export WIND_BASE=/usr/software/dev/packages/vxworks/tornado2.0/ppc
export WIND_HOST_TYPE=x86-linux
export PATH=$EPICS_BASE/bin/$HOST_ARCH:$EPICS_BASE/tools:$EPICS/extensions/bin/$HOST_ARCH:$WIND_BASE/host/$WIND_HOST_TYPE/bin:$PATH

echo "=============== environment ==============="
echo "EPICS      = $EPICS"
echo "HOST_ARCH  = $HOST_ARCH"
echo "WIND_BASE  = $WIND_BASE"
gcc --version | head -1
make --version | head -1
echo

# --- 1. Seed bin/Linux with the host-independent tools -------------------
# Many "tools" are perl or sh and work anywhere, but the build needs them
# present before it can bootstrap. Copy only the non-ELF ones; the ELF ones
# are what we are about to build.
echo "=============== seeding bin/Linux ==============="
for d in base extensions; do
    s=$EPICS/$d/bin/solaris
    t=$EPICS/$d/bin/$HOST_ARCH
    [ -d "$s" ] || continue
    mkdir -p "$t"
    n=0
    for f in "$s"/*; do
        [ -f "$f" ] || continue
        if ! file -b "$f" | grep -q ELF; then cp -p "$f" "$t/" && n=`expr $n + 1`; fi
    done
    echo "  $d: seeded $n script tools into bin/$HOST_ARCH"
done
echo

# --- 2. Build the host tools ---------------------------------------------
# Order matters: later directories use the outputs of earlier ones.
#   CROSS_COMPILER_TARGET_ARCHS=  stops base also cross-building for ppc604
#   SHRLIB_VERSION=               avoids an install rule that self-links .a
#                                 files ("Too many levels of symbolic links")
# The host tools are built native x86-64. An earlier version of this script
# had an M32 path, on the theory that the 32-bit Solaris originals mattered;
# they do not -- dbExpand's full output is byte-identical either way (333265
# bytes, md5 cefca987ee6789fce07b82677db11800 from both builds). Removed,
# because carrying it required the 32-bit devel packages, and those break the
# native C++ build by evicting libstdc++-devel.x86_64.
MAKE_ARCH_FLAGS=""

echo "=============== building host tools ==============="
echo "compiler: native $(gcc -dumpmachine)"
rc_all=0
if [ "${CLEAN_FIRST:-no}" = yes ]; then
    echo "  cleaning previous O.Linux output and bin/Linux ELF binaries"
    find $EPICS_BASE/src -type d -name 'O.Linux' -prune -exec rm -rf {} + 2>/dev/null
    for f in $EPICS_BASE/bin/Linux/*; do
        [ -f "$f" ] && file -b "$f" | grep -q ELF && rm -f "$f"
    done
fi
for d in tools include libCom toolsComm dbStatic sequencer uae; do
    if [ ! -d "$EPICS_BASE/src/$d" ]; then
        echo "  --- $d: NOT PRESENT, skipping"; continue
    fi
    printf "  --- %-12s " "$d"
    if ( cd "$EPICS_BASE/src/$d" && make CROSS_COMPILER_TARGET_ARCHS= SHRLIB_VERSION= $MAKE_ARCH_FLAGS ) \
         > /tmp/build-$d.log 2>&1; then
        echo "OK"
    else
        echo "FAILED (see /tmp/build-$d.log)"; rc_all=1
        grep -iE 'error|no rule|not found' /tmp/build-$d.log | head -6 | sed 's/^/        /'
    fi
done
echo

# --- 3. What did we get? -------------------------------------------------
echo "=============== host tools now in bin/Linux ==============="
ls $EPICS_BASE/bin/$HOST_ARCH 2>/dev/null | tr '\n' ' '; echo
echo
echo "=============== the ones the UAE build actually needs ==============="
for t in dbExpand macTest snc antelope e_flex dbToRecordtypeH dbToMenuH sf2db applSetup.pl installEpics makeMakefile.pl; do
    p=$EPICS_BASE/bin/$HOST_ARCH/$t
    if [ -f "$p" ]; then printf "  %-20s present  %s\n" "$t" "`file -b "$p" | cut -c1-40`"
    else printf "  %-20s MISSING\n" "$t"; fi
done
exit $rc_all
