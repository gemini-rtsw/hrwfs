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
# --- 32-bit host tools -------------------------------------------------
# OPTIONAL and off by default. Native x86-64 works: the full dbExpand output
# is byte-identical either way (333265 bytes, md5 cefca987ee6789fce07b82677db
# 11800 from both). Kept only because matching the Solaris tools' 32-bit-ness
# is defensible if a discrepancy ever turns up.
#
# Passed as WRAPPER SCRIPTS rather than 'ACC=gcc -m32', for two reasons:
# a make command-line assignment is one word (an embedded space makes make
# read -m32 as an option), and a command-line assignment also overrides
# in-makefile '+=' appends -- setting USR_LDFLAGS=-m32 silently wiped the
# -L paths that CONFIG.Host.UnixCommon appends, so every link failed with
# "cannot find -lDb -lCom".
MAKE_ARCH_FLAGS=""
if [ "${M32:-no}" = yes ]; then
    mkdir -p /usr/local/bin
    printf '#!/bin/sh\nexec gcc -m32 "$@"\n' > /usr/local/bin/gcc32
    # g++ -m32 does not search the i686 multilib C++ include dir on Rocky 9,
    # so bits/c++config.h is not found; add it here rather than globally.
    printf '#!/bin/sh\nexec g++ -m32 -I/usr/include/c++/11/i686-redhat-linux "$@"\n' > /usr/local/bin/g++32
    chmod +x /usr/local/bin/gcc32 /usr/local/bin/g++32
    MAKE_ARCH_FLAGS="ACC=gcc32 CCC=g++32"
fi

echo "=============== building host tools ==============="
echo "arch flags: ${MAKE_ARCH_FLAGS:-<native 64-bit>}"
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
