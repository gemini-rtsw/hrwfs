#!/bin/bash
# Per-checkout bootstrap for building hrwfs on Linux. Run once after cloning;
# then plain `gmake` builds forever after.
#
#   ./tools/linux-build/setup.sh && gmake
#
# Everything here exists because applSetup.pl stamps the checkout's ABSOLUTE
# path into the generated config, so it cannot be baked into an image.
set -e

TOP="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$TOP"

# Environment: prefer the RPM-installed profile script, fall back to the repo
# copy (identical content). Interactive shells normally have it already.
if [ -f /etc/profile.d/gem7.sh ]; then . /etc/profile.d/gem7.sh
else . "$TOP/tools/linux-build/gem-env.sh"; fi

# Scrub generated state. applSetup PRESERVES APPLIC_INSTALL from an existing
# config/CONFIG.Defs, so a checkout bootstrapped elsewhere would keep the stale
# path -- failing loudly if absent, or silently baking a wrong path into the
# generated startup scripts if it happens to exist.
rm -rf config bin lib include dbd data Distfile .applTop resource.def
find . -type d -name 'O.*' -prune -exec rm -rf {} + 2>/dev/null || true

echo "APPLIC_TOP = $TOP" > .applTop

# Capfast is dead: sch2edif fails a FlexLM check against a decommissioned
# licence server, so the .sch -> .edf -> .db chain cannot run anywhere. The
# generated databases are committed in capfast/db; seed them into the build
# directory and make them NEWER than the .sch files, or make regenerates.
mkdir -p capfast/O.$HOST_ARCH
cp capfast/db/*.db capfast/O.$HOST_ARCH/
touch capfast/O.$HOST_ARCH/*.db

# hrwfsInstall's applSetup arguments, minus the site autodetect (its
# `[ "..." -ne "" ]` is a numeric test on a string and misbehaves) and minus
# the trailing $1, which would set APPLIC_IOCPATH. The spec sets that to the
# deploy path; a developer build leaves it empty and cd's into the checkout.
SITE="${APPLIC_SITE:-MK}"
cp -f IMP_Startup$SITE.hrwfs IMP_Startup.hrwfs

# Invoke through perl explicitly: applSetup.pl's shebang is the Solaris path
# /usr/software/dev/solaris/bin/perl. A symlink there works, but not if
# /usr/software is a bind mount -- the mount hides anything the image created
# underneath it, which is exactly how this failed the first time.
APPLSETUP="$EPICS_BASE/bin/$HOST_ARCH/applSetup.pl"
[ -f "$APPLSETUP" ] || { echo "ERROR: $APPLSETUP not found -- build the host tools first" >&2; exit 1; }
# -d names the PACKAGED support-library paths, not the old shared
# /gemini/epics3.13.4/<lib>/<lib> symlinks. Those still serve the ~25 other
# GEM7 IOCs and must not be what this build resolves against; they also record
# nothing, which is the problem the packaging exists to fix.
#
# These versions also appear in hrwfs.spec's %global block. The spec is
# authoritative: it substitutes them into the generated startup scripts and
# fails the build if the result does not name the pinned versions, so the two
# cannot silently diverge.
SUP=/gemini/epics3.13.4/support
perl "$APPLSETUP" -T ppc604 -I adl -I capfast -I src -I startup \
             -I docs -I dspsrc -I par -I db \
             -d $SUP/astlib/V1-4 \
             -d $SUP/slalib/V1-9-4 \
             -d $SUP/timelib/V1-8-6 \
             -d $SUP/cfitsio/V4-1 \
             -d /gemini/dhs/dhs -S "$SITE"

# dspsrc needs Motorola's asm56000/dsplnk/cldlod/srec, which exist only as
# SPARC Solaris binaries -- it cannot build on Linux at all. Its ten generated
# .lod files are committed in dspsrc/lod and installed by the spec instead.
# (gmoscc drops `adl` from Makefile.Dirs for the same reason.)
sed -i '/^DIRS += dspsrc$/d' Makefile.Dirs

# Match production's debug format: gcc 2.7.2 emitted stabs, gcc 2.96 defaults
# to DWARF, and every deployed GEM7 object carries .stab/.stabstr. Runtime is
# unaffected either way (vxWorks ld ignores debug sections, and .symtab is
# present regardless) but the Tornado 2.0 debugger reads stabs, so DWARF would
# cost source lines and locals when debugging a crate. DEBUG_CFLAGS is in the
# CFLAGS chain and assigned nowhere, so it is a free hook; it goes in the
# generated config so a plain `make` picks it up as well as the spec's.
echo "DEBUG_CFLAGS = -gstabs" >> config/CONFIG.Defs

# applSetup failing leaves no config/, and the top-level Makefile uses
# `-include $(APPLIC_TOP)/config/CONFIG` -- so gmake would silently fall
# through to its first target, `release`, and tar the source tree instead of
# building anything, exiting 0. Fail here instead.
for f in config/CONFIG config/CONFIG.Defs config/RULES.Dirs; do
    [ -f "$f" ] || { echo "ERROR: applSetup.pl did not produce $f" >&2; exit 1; }
done

echo
echo "Setup complete for site $SITE:"
grep '^APPLIC_' config/CONFIG.Defs | sed 's/^/  /'
echo "  run 'gmake' to build."
echo
echo "NOTE: gmake needs the environment too -- HOST_ARCH especially, or it"
echo "      resolves CONFIG_HOST_ARCH.unsupported and stops. Source"
echo "      tools/linux-build/gem-env.sh (or /etc/profile.d/gem7.sh) first."
