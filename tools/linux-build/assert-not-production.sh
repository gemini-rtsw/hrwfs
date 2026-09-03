#!/bin/sh
# Safety gate: run between `applSetup.pl` and `gmake` on the old build host.
# Exits non-zero if this build could write into /gemini.
#
#   sh /var/tmp/assert-not-production.sh   &&   gmake
#
# WHY THIS EXISTS. The UAE build does not merely stamp APPLIC_INSTALL into
# generated files -- it INSTALLS INTO IT ("Installing $APPLIC_INSTALL/./data/...").
# applSetup.pl derives it from `pwd`, and PRESERVES whatever is already in an
# existing config/CONFIG.Defs. So a build started in the wrong directory, or a
# checkout carrying someone else's CONFIG.Defs, writes straight into the
# production tree -- and the default `gmake` target is buildInstall, so it
# happens on the very first build, with no separate install step to forget.
#
# Nothing here writes anything. Read-only checks only.
set -u

FAIL=0
say() { echo "  $*"; }
bad() { echo "  FAIL: $*" >&2; FAIL=1; }

echo "=== safety gate: can this build write to /gemini? ==="

TOP=`pwd`
say "cwd = $TOP"

case "$TOP" in
    /gemini/*) bad "cwd is under /gemini. Build in a private directory." ;;
esac

if [ ! -f .applTop ]; then
    bad ".applTop missing -- run applSetup.pl first"
else
    AT=`sed -n 's/^ *APPLIC_TOP *= *//p' .applTop`
    say ".applTop APPLIC_TOP = $AT"
    case "$AT" in
        /gemini/*) bad "APPLIC_TOP names /gemini. Every Makefile reads
        \$(APPLIC_TOP)/config/CONFIG, so this build would inherit
        production's CONFIG.Defs -- including its APPLIC_INSTALL." ;;
        "")        bad "APPLIC_TOP is empty" ;;
    esac
    if [ "$AT" != "$TOP" ]; then
        bad "APPLIC_TOP ($AT) is not cwd ($TOP)"
    fi
fi

if [ ! -f config/CONFIG.Defs ]; then
    bad "config/CONFIG.Defs missing -- run applSetup.pl first"
else
    echo
    say "config/CONFIG.Defs APPLIC_* lines:"
    grep '^APPLIC_' config/CONFIG.Defs | sed 's/^/    /'
    echo

    INST=`sed -n 's/^APPLIC_INSTALL *= *//p' config/CONFIG.Defs | sed 's/ *$//'`
    IOCP=`sed -n 's/^APPLIC_IOCPATH *= *//p' config/CONFIG.Defs | sed 's/ *$//'`

    # THE critical one: gmake installs here.
    case "$INST" in
        /gemini/*) bad "APPLIC_INSTALL = $INST -- gmake would INSTALL INTO PRODUCTION.
        Delete config/ and re-run applSetup.pl from this directory. applSetup
        preserves a stale APPLIC_INSTALL, so editing it by hand is not enough
        if config/ came from elsewhere." ;;
        ""|".")    say "APPLIC_INSTALL = '${INST}' (relative -- installs into cwd) OK" ;;
        "$TOP")    say "APPLIC_INSTALL = cwd  OK" ;;
        *)         bad "APPLIC_INSTALL = $INST -- neither cwd nor relative.
        Check where that points before building." ;;
    esac

    # Stamped into the generated scripts, not written to -- but a production
    # value here means the checkout inherited someone else's config.
    case "$IOCP" in
        /gemini/*) bad "APPLIC_IOCPATH = $IOCP -- production. Nothing installs
        there, but its presence means config/ was inherited, so APPLIC_INSTALL
        cannot be trusted either. Delete config/ and re-run applSetup.pl." ;;
        "")        say "APPLIC_IOCPATH = (empty)  OK" ;;
        *)         say "APPLIC_IOCPATH = $IOCP  (not /gemini, OK)" ;;
    esac
fi

# The Distfile is what rdist would push. Creating it is harmless; running
# rdist with it is not. Show what it targets so there are no surprises.
if [ -f Distfile ]; then
    echo
    say "Distfile exists. It targets:"
    grep -i 'DIST_HOST\|DIST_PATH\|->' Distfile 2>/dev/null | head -10 | sed 's/^/    /'
    say "Do NOT run 'gmake rdist' -- that is the step that writes to production."
fi

echo
if [ "$FAIL" -ne 0 ]; then
    echo "=== BLOCKED. Do not run gmake. ===" >&2
    exit 1
fi
echo "=== clear: gmake will stay inside $TOP ==="
exit 0
