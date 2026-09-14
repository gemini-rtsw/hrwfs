# Wind River Tornado 2.0.2 PowerPC cross-toolchain for Linux x86 hosts.
#
# Host tools are ANL's rebuild of Wind River's GPL'd GCC
# (https://epics.anl.gov/base/tornado-linux.php). NOTE: ANL publishes that
# rebuild for Tornado 2.2 (gcc 2.96) only -- there is no 2.0 build, and the
# GNU sources are not on the Gemini Tornado 2.0.2 installation (host/src is
# 282 KB of host utilities and WindView, not a compiler). So this package
# pairs the 2.2-era gcc 2.96 host tools with the Tornado 2.0.2 TARGET headers
# and config, which is the combination verified to build hrwfs:
#
#   19/19 sources compile, all 10 loadable modules link, and the objects are
#   equivalent to production's -- identical .text/.data sizes, differing only
#   in that gcc 2.96 applies -mlongcall consistently where gcc 2.7.2 emitted
#   some direct bl. See MIGRATION-PLAN.md 0b.
#
# The target headers are what make this a Tornado 2.0 package: they are the
# vxWorks 5.4 API the crate actually runs. Installs at the WIND_BASE path the
# GEM7 build system expects.

%global _build_id_links none
%global __os_install_post %{nil}
%global debug_package %{nil}
%global windbase /usr/software/dev/packages/vxworks/tornado2.0/ppc

Name:           gem-tornado20-linux
Version:        2.0.2
Release:        1%{?dist}
Summary:        Tornado 2.0.2 ppc target headers + x86-linux cross-toolchain
License:        GPL (host tools) / Proprietary (target headers, org-internal)
AutoReqProv:    no
# ccppc and friends are 32-bit i386 ELF
Requires:       glibc(x86-32)

%description
The vxWorks 5.4 / Tornado 2.0.2 target headers and BSP config from the Gemini
installation, plus ccppc/ldppc/arppc/objdumpppc etc. as 32-bit Linux binaries
(ANL's rebuild of the Wind River GNU tools, gcc 2.96). Installs under
%{windbase}, the WIND_BASE path the GEM7 build system names.

Solaris and Win32 host trees are deliberately excluded: the Solaris tools are
SPARC binaries that cannot run here, and the Win32 tree in the Gemini install
is a MIPS-target Tornado IDE with no PowerPC compiler in it.

%install
mkdir -p %{buildroot}%{windbase}
cp -a %{trees}%{windbase}/host \
      %{trees}%{windbase}/target \
      %{buildroot}%{windbase}/
# Only the x86-linux host tools belong here.
rm -rf %{buildroot}%{windbase}/host/sun4-solaris2 \
       %{buildroot}%{windbase}/host/x86-win32 \
       %{buildroot}%{windbase}/host/parisc-hpux10

# Fail the build rather than ship a toolchain that cannot compile: these are
# the binaries the EPICS rules invoke by name.
for f in ccppc ldppc arppc nmppc objcopyppc objdumpppc; do
    [ -x "%{buildroot}%{windbase}/host/x86-linux/bin/$f" ] || {
        echo "ERROR: missing host/x86-linux/bin/$f" >&2; exit 1; }
done
# ...and the target headers, without which nothing compiles at all.
[ -f "%{buildroot}%{windbase}/target/h/vxWorks.h" ] || {
    echo "ERROR: missing target/h/vxWorks.h" >&2; exit 1; }

%files
%{windbase}

%changelog
* Thu Sep 10 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 2.0.2-1
- Initial packaging for the hrwfs Linux rehost. Target headers are Tornado
  2.0.2 (vxWorks 5.4) from the Gemini install; host tools are ANL's
  gnu-tools.tor2_2-ppc-rhel5.tgz, sha256
  b9881437f7f0c1cdd1acdda5a2e0b02515050b9805b5604b001fb21a4df30a2e.
