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

# Resolve the kernel symlink into a real file at the path the boot parameters
# name. In the source tree mv2700/vxWorks is a link, set on 31 Jul 2008, to
# BCvxWorks -- one of four images sitting beside it (STNDvxWorks, BCvxWorks,
# BCNETvxWorks, T2vxWorks). BC is Bancomm: STNDvxWorks carries no Bancomm
# symbols and the other three do, and hrwfs needs it (timeClockInit,
# TSconfigure, geminiBancommDev.dbd). So the choice was deliberate, but it was
# recorded nowhere and had to be reconstructed from symbol tables.
#
# gmoscc's gem-vxworks-tornado22 ships a real file at the equivalent path with
# its hash in the changelog; this makes the 2.0 package match. After this the
# boot parameter names a file rather than a link, rpm -V detects it changing,
# and the changelog below says exactly which image it is. The other three
# remain in the package for anyone who needs them.
KDIR=%{buildroot}/gemini/external/vxWorks/tornado2.0/mv2700
for f in vxWorks vxWorks.sym; do
    [ -L "$KDIR/$f" ] || continue
    tgt=$(readlink "$KDIR/$f")
    rm -f "$KDIR/$f"
    cp -a "$KDIR/$tgt" "$KDIR/$f"
    echo "resolved mv2700/$f -> $tgt (now a real file)"
done

chown -R root:root %{buildroot}/gemini/external/vxWorks

# The crate's boot parameters name this file. It must be a REAL file, not the
# link it is in the source tree -- a link would put the choice of kernel back
# outside the package.
[ -L "%{buildroot}/gemini/external/vxWorks/tornado2.0/mv2700/vxWorks" ] && {
    echo "ERROR: mv2700/vxWorks is still a symlink" >&2; exit 1; }
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
- mv2700/vxWorks and vxWorks.sym are installed as REAL files resolved from
  the source tree's symlinks, which pointed at BCvxWorks (set 31 Jul 2008).
  The images carry no internal version marker, so these hashes are the only
  way to tell one build from another:
    vxWorks     (BCvxWorks)     1435143  sha256 2b838e55154418d2140529d24c93da9b98e6af2d09d7bb6d4d13aebb5660b7c2
    vxWorks.sym (BCvxWorks.sym)  196952  sha256 57bbd4f91358003be28e8cc1818820d669e499f234b7e32c9bdc818a0777772c
  STNDvxWorks, BCNETvxWorks and T2vxWorks remain in the package. T2vxWorks
  (2005) is newer and also has Bancomm support; why the crate is not on it is
  not recorded anywhere.
