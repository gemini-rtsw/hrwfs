#!/bin/bash
# Collect everything the hrwfs Linux cross-build needs, from the hosts that
# still have it. Nothing is installed and nothing is written outside /var/tmp.
#
# hrwfs shares NONE of gmoscc's artifacts: gmoscc is GEM8.6 / EPICS 3.13.9 /
# ppc604_long, hrwfs is GEM7 / EPICS 3.13.4 / ppc604 and additionally needs
# cfitsio, the DHS vxWorks client libraries and a Motorola DSP56000 assembler.
# So this is a from-scratch pass, per docs/README-LINUX-REHOST.md in gmoscc.
#
# Three modes, run on three different kinds of host:
#
#   inventory   the Solaris build host for hrwfs  -- answers questions, copies nothing
#   buildenv    the same host                     -- tars the EPICS/Tornado/DSP trees
#   runtime     the boot server exporting /gemini -- tars the trees crates ld at boot
#   deployed    the same boot server              -- tars the LIVE hrwfs tree (escrow)
#
# Usage, per host:
#   scp tools/linux-build/stage-buildenv.sh <host>:/var/tmp/
#   ssh <host> 'sh /var/tmp/stage-buildenv.sh inventory'    # read this first
#   ssh <host> 'sh /var/tmp/stage-buildenv.sh buildenv'
#   scp <host>:/var/tmp/hrwfs-stage/*.tar.gz .
#
# Solaris notes: /bin/sh, no `tar z`, and /tmp is swap-backed -- hence
# /var/tmp and the explicit gzip pipes.

OUT=/var/tmp/hrwfs-stage
mkdir -p $OUT

case "${1:-inventory}" in

inventory)
    echo "=== host ==="; uname -a; hostname
    echo
    echo "=== the GEM environment hrwfs builds under ==="
    echo "# Which setup alias/file? gmoscc used 'GEM8.6' -> ~/.gem8.6."
    echo "# hrwfs points at GEM7 / EPICS 3.13.4, so expect a different one."
    ls -la $HOME/.gem* 2>/dev/null
    echo
    echo "EPICS      = ${EPICS:-<unset>}"
    echo "EPICS_BASE = ${EPICS_BASE:-<unset>}"
    echo "HOST_ARCH  = ${HOST_ARCH:-<unset>}"
    echo "WIND_BASE  = ${WIND_BASE:-<unset>}"
    echo "WIND_HOST_TYPE = ${WIND_HOST_TYPE:-<unset>}"
    echo
    echo "=== cross-compiler ==="
    which ccppc 2>/dev/null && ccppc -v 2>&1 | tail -3
    file `which ccppc 2>/dev/null` 2>/dev/null
    echo
    echo "=== Tornado version + host dirs (need to know if 2.0 or 2.2) ==="
    echo "$WIND_BASE:"; ls $WIND_BASE 2>/dev/null
    ls $WIND_BASE/host 2>/dev/null
    cat $WIND_BASE/../.wind_version 2>/dev/null
    echo
    echo "=== UAE / EPICS tools present? ==="
    for t in applSetup.pl gmake snc dbExpand e2db sch2edif adl2dl edd macTest antelope e_flex; do
        printf "%-14s " $t; which $t 2>/dev/null || echo "MISSING"
    done
    echo
    echo "=== the DSP toolchain (dspsrc needs this; nothing else does) ==="
    ls -la /usr/software/dev/packages/asm56000 2>/dev/null || echo "  NOT HERE"
    for b in asm56000 dsplnk cldlod srec; do
        f=/usr/software/dev/packages/asm56000/$b
        [ -f "$f" ] && { printf "%-10s " $b; file "$f"; }
    done
    echo "# If these are SPARC ELF, dspsrc cannot build on Linux and the"
    echo "# generated bin/asm56000/*.lod files have to be committed instead"
    echo "# -- same mitigation as gmoscc used for the dead Capfast step."
    echo
    echo "=== tree sizes (how big are the tarballs going to be) ==="
    du -sk $EPICS 2>/dev/null
    du -sk $WIND_BASE/target 2>/dev/null
    du -sk /usr/software/dev/packages/asm56000 2>/dev/null
    ;;

buildenv)
    echo "Staging the build environment into $OUT (Solaris tar + gzip pipes)."
    [ -n "${EPICS:-}" ]     || { echo "ERROR: \$EPICS unset -- source the GEM environment first"; exit 1; }
    [ -n "${WIND_BASE:-}" ] || { echo "ERROR: \$WIND_BASE unset -- source the GEM environment first"; exit 1; }

    echo "-> gem-config.tar.gz  (the ~/.gemN setup file)"
    tar cf - $HOME/.gem* 2>/dev/null | gzip -c > $OUT/gem-config.tar.gz

    echo "-> epics.tar.gz       ($EPICS -- base + extensions, the build tree)"
    tar cf - $EPICS | gzip -c > $OUT/epics.tar.gz

    echo "-> wind-target.tar.gz ($WIND_BASE/target/{h,config} -- headers only;"
    echo "                       the Linux cross-tools come from ANL, not here)"
    tar cf - $WIND_BASE/target/h $WIND_BASE/target/config | gzip -c > $OUT/wind-target.tar.gz

    if [ -d /usr/software/dev/packages/asm56000 ]; then
        echo "-> asm56000.tar.gz    (DSP56000 assembler -- keep even if SPARC-only)"
        tar cf - /usr/software/dev/packages/asm56000 | gzip -c > $OUT/asm56000.tar.gz
    fi

    echo; ls -l $OUT
    echo
    echo "These tarballs are the only copies of some of this software."
    echo "Keep them -- they are the escrow for anything the RPMs later omit."
    ;;

runtime)
    # The trees a crate ld's at boot, read straight out of the startup scripts.
    # Each becomes a dependency RPM; see MIGRATION-PLAN.md.
    echo "Staging the runtime /gemini trees into $OUT."
    stage() {
        name=$1; shift
        for p in "$@"; do
            # No -e: SVR4 test does not have it. -f or -d covers what we stage.
            if [ ! -f "$p" ] && [ ! -d "$p" ]; then
                echo "  MISSING: $p"; return 1
            fi
        done
        echo "-> $name.tar.gz"
        tar cf - "$@" 2>/dev/null | gzip -c > $OUT/$name.tar.gz
    }

    # gem7-epics-runtime: iocCore, seq, pvload for ppc604
    stage gem7-epics-runtime \
        /gemini/external/GEM7/base/bin/ppc604 \
        /gemini/external/GEM7/extensions/bin/ppc604

    # hrwfs-deplibs: astlib/slalib/timelib/cfitsio, EPICS 3.13.4 generation.
    # NOTE the double directory: /gemini/epics3.13.4/slalib/slalib -- the inner
    # one is the version-selecting symlink, exactly as gmoscc's was.
    stage hrwfs-deplibs \
        /gemini/epics3.13.4/astlib \
        /gemini/epics3.13.4/slalib \
        /gemini/epics3.13.4/timelib \
        /gemini/epics3.13.4/cfitsio

    # hrwfs-dhs-vxlibs: the DHS client libraries, arch mv2700T2 (not ppc604)
    stage hrwfs-dhs-vxlibs \
        /gemini/dhs/dhs/external/lib/mv2700T2 \
        /gemini/dhs/dhs/lib/mv2700T2

    # vxUsers + the kernel. gem-vxworks-tornado22 already ships tornado2.0
    # and tornado2.2 for gmoscc; staged here only to confirm this server's
    # copy is the same one.
    stage gem-vxworks \
        /gemini/external/vxWorks/tornado2.0 \
        /gemini/external/vxWorks/tornado2.2

    echo; ls -l $OUT
    echo
    echo "Record the version symlinks -- the RPM pins what these point at:"
    for l in /gemini/epics3.13.4/*/*; do
        [ -h "$l" ] && printf "  %-46s -> %s\n" "$l" "`readlink $l`"
    done
    ;;

deployed)
    # The LIVE hrwfs tree. This is escrow, not a build input -- it holds three
    # things that no build can reproduce any more:
    #   data/hrwfsTop.db, data/hrwfsSadTop.db   Capfast output (sch2edif is dead)
    #   bin/*/local                             no *.vws in the repo generates it
    #   bin/asm56000/*.lod                       DSP output (Solaris-only assembler)
    # plus bin/ppc604/* for the object-level comparison against the Linux build.
    D="${2:-}"
    if [ -z "$D" ]; then
        echo "usage: $0 deployed /path/to/live/hrwfs"
        echo
        echo "Looking for candidates under /gemini:"
        ls -d /gemini/*/hrwfs* /gemini/*/*/hrwfs* 2>/dev/null
        exit 1
    fi
    [ -d "$D" ] || { echo "ERROR: $D is not a directory"; exit 1; }
    echo "Staging the deployed tree $D"
    echo
    echo "=== what is actually there ==="
    ls -la "$D"
    echo
    echo "=== the three irreproducible things ==="
    ls -la "$D"/data/hrwfsTop.db "$D"/data/hrwfsSadTop.db 2>/dev/null || echo "  .db MISSING"
    ls -la "$D"/bin/*/local 2>/dev/null                              || echo "  local MISSING"
    ls -la "$D"/bin/asm56000/ 2>/dev/null                            || echo "  asm56000/ MISSING"
    echo
    echo "-> hrwfs-deployed.tar.gz"
    ( cd "$D" && tar cf - bin data dbd include RELEASE.NOTES 2>/dev/null ) \
        | gzip -c > $OUT/hrwfs-deployed.tar.gz
    ls -l $OUT/hrwfs-deployed.tar.gz
    ;;

*)
    echo "usage: $0 {inventory|buildenv|runtime|deployed [path]}"; exit 1 ;;
esac
