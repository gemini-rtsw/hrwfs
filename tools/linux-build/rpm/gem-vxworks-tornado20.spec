# Prebuilt VxWorks 5.4 kernel images for Gemini VME crates on Tornado 2.0, at
# their runtime path under /gemini/external. Org-internal only.
#
# NOT a build dependency, unlike gem-tornado20-linux. That one ships the
# cross-toolchain to $WIND_BASE so hrwfs can be compiled; this ships the
# kernel the crate loads over the wire at boot, which is a different file at a
# different path for a different purpose. It belongs on the NFS/rsh boot
# server, not on a build host.
#
# The hrwfs crate's boot parameters name it directly:
#
#   file name (f): /gemini/external/vxWorks/tornado2.0/mv2700/vxWorks
#
# The images carry no internal version marker, which is the reason for
# packaging rather than copying: after this, `rpm -q` identifies which kernel
# a crate loaded.
#
# This package does NOT ship tornado2.0/vxUsers. gem-vxworks-tornado22 owns
# a copy of that file under tornado2.2 (gmoscc reads it from there), and the
# hrwfs startup reads it from tornado2.0 -- so this package owns the
# tornado2.0 tree including vxUsers, and the two packages have no overlapping
# paths. Verified: gem-vxworks-tornado22's %%files lists only
# /gemini/external/vxWorks/tornado2.2.

%global _binaries_in_noarch_packages_terminate_build 0
%global _build_id_links none
%global __os_install_post %{nil}
%global debug_package %{nil}

Name:           gem-vxworks-tornado20
Version:        2.0.2
Release:        1%{?dist}
Summary:        Prebuilt VxWorks 5.4 kernel images for Tornado 2.0 VME targets
License:        Proprietary (Wind River / Gemini) -- org-internal
AutoReqProv:    no
# noarch: one PowerPC VxWorks image and no host binaries at all, so tagging it
# to the build host's arch is wrong and actively harmful -- a build on an
# arm64 laptop would produce a package that will not install on the x86_64
# boot server.
BuildArch:      noarch

%description
VxWorks 5.4 boot images built under Tornado 2.0, installed at
/gemini/external/vxWorks/tornado2.0 -- the path Gemini VME boot parameters
name directly, e.g.

    file name (f): /gemini/external/vxWorks/tornado2.0/mv2700/vxWorks

Carries the mv2700 (MVME2700) BSP plus vxUsers, the shell login definitions
the hrwfs startup script reads. Install on the boot server that exports
/gemini. The crate fetches the image over rsh or TFTP depending on its boot
flags; neither needs this package on the crate itself.

%install
mkdir -p %{buildroot}/gemini/external/vxWorks
cp -a %{trees}/gemini/external/vxWorks/tornado2.0 \
      %{buildroot}/gemini/external/vxWorks/
chown -R root:root %{buildroot}/gemini/external/vxWorks

# The crate's boot parameters name this file. Without it there is no boot.
[ -f "%{buildroot}/gemini/external/vxWorks/tornado2.0/mv2700/vxWorks" ] || {
    echo "ERROR: missing mv2700/vxWorks -- this is the kernel the crate boots" >&2; exit 1; }
# hrwfs's local.vws and startup_MINIMAL both read this; without it the crate
# loads its kernel and then fails on "can't open input .../vxUsers".
[ -f "%{buildroot}/gemini/external/vxWorks/tornado2.0/vxUsers" ] || {
    echo "ERROR: missing vxUsers -- the startup script reads this" >&2; exit 1; }

%files
%defattr(-,root,root,-)
/gemini/external/vxWorks/tornado2.0

%changelog
* Mon Sep 14 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 2.0.2-1
- Initial packaging of the mv2700 VxWorks 5.4 image and vxUsers, taken from
  the Gemini /gemini tree. Packaged so the boot kernel a crate loads is
  identifiable by rpm -q rather than being an undated binary copied between
  hosts.
