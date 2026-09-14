# EPICS 3.13.4 GEM7 BUILD tree: UAE config/rules, headers, dbd, templates, the
# Linux host tools, and the prebuilt ppc604 target objects.
#
# Not to be confused with gem7-epics-runtime, which installs the ppc604
# binaries the CRATE loads at /gemini/external/GEM7. This one is what gmake
# needs on the build host.
#
# base/bin/ppc604 matters more than it looks: db/Makefile.Vx assembles
# gemini.Support from ~144 prebuilt record/device .o files there (via the
# *LIBOBJS lists), so they are build INPUTS, not products. They are gcc 2.7.2
# objects and are shipped verbatim.
#
# VERSIONING CONVENTION: packages tied to a GEM software-tree version carry
# the version in the name and install under version-specific paths, so GEM7
# and GEM8.6 co-install without conflicts.

%global _binaries_in_noarch_packages_terminate_build 0
%global _build_id_links none
%global __os_install_post %{nil}
%global debug_package %{nil}
%global epicsdir /usr/software/dev/packages/epics/epics3.13.4GEM7

Name:           gem-epics3134gem7
Version:        3.13.4
Release:        1%{?dist}
Summary:        EPICS 3.13.4 GEM7 build tree (Linux host tools + ppc604 objects)
License:        EPICS Open License / Proprietary Gemini additions (org-internal)
AutoReqProv:    no
Requires:       gem-tornado20-linux
Requires:       perl, tcsh, make

%description
The GEM7 EPICS build tree at %{epicsdir}: UAE config and rules, headers, dbd,
templates, the HOST_ARCH=Linux tools (dbExpand, macTest, snc, antelope,
e_flex, dbToRecordtypeH) and the prebuilt ppc604 target objects that
gemini.Support is assembled from. Ships /etc/profile.d/gem7.{sh,csh} so a
login shell gets the environment, and the perl symlink applSetup.pl's Solaris
shebang expects.

%install
mkdir -p %{buildroot}%{epicsdir}/base %{buildroot}%{epicsdir}/extensions
cd %{trees}%{epicsdir}
for d in config include dbd templates tools startup bin/Linux bin/ppc604 lib/Linux; do
    [ -e "base/$d" ] || continue
    mkdir -p "%{buildroot}%{epicsdir}/base/$(dirname $d)"
    cp -a "base/$d" "%{buildroot}%{epicsdir}/base/$d"
done
for d in include config bin/Linux bin/ppc604 lib/Linux; do
    [ -e "extensions/$d" ] || continue
    mkdir -p "%{buildroot}%{epicsdir}/extensions/$(dirname $d)"
    cp -a "extensions/$d" "%{buildroot}%{epicsdir}/extensions/$d"
done

# Solaris-era build litter, and the Solaris host binaries which cannot run here.
find %{buildroot}%{epicsdir} -type d -name 'O.*' -prune -exec rm -rf {} + 2>/dev/null || :
rm -rf %{buildroot}%{epicsdir}/base/bin/solaris %{buildroot}%{epicsdir}/extensions/bin/solaris \
       %{buildroot}%{epicsdir}/base/lib/solaris

# The cross-build site override. Upstream CONFIG_HOST_ARCH.Linux sets
# WIND_HOST_TYPE=Linux and CONFIG.Vx derives VX_GNU from it, so without this
# the build looks for host/Linux/bin/ccppc while ANL's toolchain unpacks to
# host/x86-linux -- "Error 127". It cannot be fixed from the environment,
# because a makefile assignment beats the environment. Shipped here so no
# checkout has to patch anything.
install -Dpm 0644 %{trees}/patches/CONFIG_SITE.Vx.Linux.ppc604 \
    %{buildroot}%{epicsdir}/base/config/CONFIG_SITE.Vx.Linux.ppc604

# applSetup.pl's shebang names the Solaris perl path.
mkdir -p %{buildroot}/usr/software/dev/solaris/bin
ln -sf /usr/bin/perl %{buildroot}/usr/software/dev/solaris/bin/perl

mkdir -p %{buildroot}/etc/profile.d
cat > %{buildroot}/etc/profile.d/gem7.sh <<'EOF'
# GEM7 EPICS 3.13.4 / Tornado 2.0.2 build environment (gem-epics3134gem7 RPM)
export EPICS=/usr/software/dev/packages/epics/epics3.13.4GEM7
export EPICS_BASE=$EPICS/base
export HOST_ARCH=Linux
export WIND_BASE=/usr/software/dev/packages/vxworks/tornado2.0/ppc
export WIND_HOST_TYPE=x86-linux
export PATH=$EPICS/base/bin/$HOST_ARCH:$EPICS/extensions/bin/$HOST_ARCH:$WIND_BASE/host/$WIND_HOST_TYPE/bin:$PATH
EOF
cat > %{buildroot}/etc/profile.d/gem7.csh <<'EOF'
setenv EPICS /usr/software/dev/packages/epics/epics3.13.4GEM7
setenv EPICS_BASE $EPICS/base
setenv HOST_ARCH Linux
setenv WIND_BASE /usr/software/dev/packages/vxworks/tornado2.0/ppc
setenv WIND_HOST_TYPE x86-linux
setenv PATH $EPICS/base/bin/$HOST_ARCH\:$EPICS/extensions/bin/$HOST_ARCH\:$WIND_BASE/host/$WIND_HOST_TYPE/bin\:$PATH
EOF

# A missing host tool is a build that fails much later and less clearly.
for f in dbExpand macTest snc antelope e_flex dbToRecordtypeH applSetup.pl; do
    [ -e "%{buildroot}%{epicsdir}/base/bin/Linux/$f" ] || {
        echo "ERROR: missing base/bin/Linux/$f -- host tools not built?" >&2; exit 1; }
done
# gemini.Support is assembled from these; without them db/ cannot link.
[ -f "%{buildroot}%{epicsdir}/base/bin/ppc604/aiRecord.o" ] || {
    echo "ERROR: missing base/bin/ppc604 record objects" >&2; exit 1; }
[ -f "%{buildroot}%{epicsdir}/base/dbd/geminiRecLIBOBJS" ] || {
    echo "ERROR: missing dbd/geminiRecLIBOBJS" >&2; exit 1; }

%files
%{epicsdir}
/usr/software/dev/solaris/bin/perl
/etc/profile.d/gem7.sh
/etc/profile.d/gem7.csh

%changelog
* Thu Sep 10 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 3.13.4-1
- Initial packaging for the hrwfs Linux rehost. Host tools built for
  HOST_ARCH=Linux from the tree's own source (all seven directories, gcc
  11.5.0, no patches). Carries CONFIG_SITE.Vx.Linux.ppc604 so the cross
  toolchain is found at host/x86-linux.
