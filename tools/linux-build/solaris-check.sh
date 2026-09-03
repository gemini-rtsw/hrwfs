#!/bin/sh
# READ-ONLY inventory of the old hrwfs build machine. Writes nothing anywhere
# except /var/tmp, touches nothing under /gemini, and runs no build.
#
#   scp tools/linux-build/solaris-check.sh <buildhost>:/var/tmp/
#   ssh <buildhost> 'sh /var/tmp/solaris-check.sh' 2>&1 | tee hrwfs-check.txt
#
# Then, if it finds a working copy with db/:
#   ssh <buildhost> 'sh /var/tmp/solaris-check.sh grab /path/to/that/workingcopy'
#   scp <buildhost>:/var/tmp/hrwfs-stage/hrwfs-missing-dirs.tar.gz .
#
# Solaris: /bin/sh, no `tar z`, /tmp is swap-backed -- hence /var/tmp and the
# explicit gzip pipes.

OUT=/var/tmp/hrwfs-stage
PROD=/gemini/epics3.13.4/hrwfs

case "${1:-check}" in

check)
    echo "=== host ==="
    uname -a; hostname; id
    echo

    echo "=== CAN THIS ACCOUNT WRITE TO PRODUCTION? ==="
    # test -w only stats; it does not create anything.
    for d in /gemini /gemini/epics3.13.4 $PROD $PROD/V3-8-5 \
             $PROD/V3-8-5/bin $PROD/V3-8-5/bin/ppc604 $PROD/V3-8-5/data; do
        if [ -e "$d" ]; then
            if [ -w "$d" ]; then echo "  WRITABLE   $d   <-- danger"
            else                 echo "  read-only  $d"; fi
        else
            echo "  absent     $d"
        fi
    done
    echo
    echo "  If any line says WRITABLE, the safest thing is to do the build as an"
    echo "  account that cannot write there. If that is not possible, the guards"
    echo "  in the build recipe are the only thing standing between a typo and"
    echo "  an overwritten production tree."
    echo

    echo "=== production tree, for reference (not touched) ==="
    ls -la $PROD 2>/dev/null
    echo

    echo "=== hrwfs working copies / build dirs on this host ==="
    # Anything that looks like an hrwfs checkout: has src/ and capfast/.
    for base in $HOME /home /export/home /usr/local/src /opt /var/tmp /gemsoft; do
        [ -d "$base" ] || continue
        find "$base" -type d -name capfast 2>/dev/null \
          | while read c; do
                w=`dirname "$c"`
                [ -d "$w/src" ] || continue
                case "$w" in $PROD*) continue ;; esac
                echo
                echo "  --- $w"
                echo "      .applTop:  `cat $w/.applTop 2>/dev/null || echo '(none)'`"
                printf "      db/:       "
                if [ -d "$w/db" ]; then ls "$w/db" | tr '\n' ' '; echo; else echo "ABSENT"; fi
                printf "      local.vws: "
                [ -f "$w/startup/local.vws" ] && echo "present" || echo "ABSENT"
                printf "      capfast .db: "
                ls "$w/capfast"/*.db 2>/dev/null | wc -l
                echo "      RELEASE.NOTES tail: `tail -3 $w/RELEASE.NOTES 2>/dev/null | tr '\n' ' '`"
                if [ -f "$w/config/CONFIG.Defs" ]; then
                    echo "      config/CONFIG.Defs APPLIC_* lines:"
                    grep '^APPLIC_' "$w/config/CONFIG.Defs" | sed 's/^/        /'
                fi
            done
    done
    echo

    echo "=== the two directories missing from the SVN tag, anywhere on this host ==="
    find / -type d -name db -path '*hrwfs*' 2>/dev/null | head -20
    find / -name 'local.vws' -path '*hrwfs*' 2>/dev/null | head -20
    echo

    echo "=== GEM environment (which alias/file does hrwfs use?) ==="
    ls -la $HOME/.gem* 2>/dev/null
    echo "  (hrwfs points at GEM7 / EPICS 3.13.4 -- expect a .gem7-ish file,"
    echo "   not the .gem8.6 gmoscc used)"
    echo

    echo "=== is asm56000 here, and what is it? ==="
    ls -la /usr/software/dev/packages/asm56000 2>/dev/null || echo "  NOT HERE"
    for b in asm56000 dsplnk cldlod srec; do
        f=/usr/software/dev/packages/asm56000/$b
        [ -f "$f" ] && { printf "  %-10s " $b; file "$f"; }
    done
    echo

    echo "=== capfast licence (expected dead; confirming, not fixing) ==="
    echo "  CAPFAST_LMHOST=${CAPFAST_LMHOST:-<unset>}"
    which sch2edif 2>/dev/null || echo "  sch2edif not on PATH"
    ;;

grab)
    W="$2"
    if [ -z "$W" ]; then echo "usage: $0 grab /path/to/hrwfs/workingcopy"; exit 1; fi
    [ -d "$W/src" ] || { echo "ERROR: $W does not look like an hrwfs checkout"; exit 1; }
    case "$W" in
      /gemini/*) echo "ERROR: refusing to read from under /gemini -- give me a build dir"; exit 1 ;;
    esac
    mkdir -p $OUT

    echo "Grabbing, from $W, only what the SVN tag does not have."
    echo "Read-only: this is a tar, nothing is modified."
    echo

    # db/ is the prize: it builds dbd/gemini.dbd and the gemini.Support
    # library, both of which startup loads and neither of which the tag can
    # produce. startup/local.vws is the other missing source file.
    SETS=""
    [ -d "$W/db" ]                 && SETS="$SETS db"
    [ -f "$W/startup/local.vws" ]  && SETS="$SETS startup/local.vws"
    [ -f "$W/config/CONFIG.Defs" ] && SETS="$SETS config/CONFIG.Defs"
    [ -f "$W/config/CONFIG" ]      && SETS="$SETS config/CONFIG"
    [ -f "$W/.applTop" ]           && SETS="$SETS .applTop"
    [ -f "$W/UAE.dist" ]           && SETS="$SETS UAE.dist"
    [ -f "$W/Distfile" ]           && SETS="$SETS Distfile"
    [ -f "$W/resource.def" ]       && SETS="$SETS resource.def"
    ls "$W"/capfast/*.db >/dev/null 2>&1 && SETS="$SETS capfast"

    [ -n "$SETS" ] || { echo "Nothing missing found in $W"; exit 1; }
    echo "including:$SETS"
    ( cd "$W" && tar cf - $SETS ) | gzip -c > $OUT/hrwfs-missing-dirs.tar.gz
    echo
    ls -l $OUT/hrwfs-missing-dirs.tar.gz
    echo
    echo "Also worth having for the object-level comparison later:"
    echo "  ( cd $W && tar cf - src/O.* db/O.* ) | gzip -c > $OUT/hrwfs-objs.tar.gz"
    ;;

*)
    echo "usage: $0 {check|grab <workingcopy>}"; exit 1 ;;
esac
