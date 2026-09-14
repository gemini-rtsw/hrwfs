# DHS client libraries for vxWorks, as loaded by the hrwfs crate at boot.
#
# The startup script does
#
#   ld < /gemini/dhs/dhs/external/lib/mv2700T2/lib{ers,imp,sds}.o
#   ld < /gemini/dhs/dhs/lib/mv2700T2/lib{gen,dhs}.a
#
# Note the architecture is mv2700T2, not ppc604: these are DRAMA and DHS
# client objects built for the MVME2700 board, and they are the only part of
# the boot set that does not follow the ppc604 naming.
#
# NO VERSION EXISTS TO PACKAGE. /gemini/dhs/dhs is a real directory, not a
# version-selecting symlink, sitting beside seven siblings (dhs-0.19,
# dhs-0.19b, dhs-0.19bDebug, dhs-0.19c, dhs-0.19c-hbf, dhs-2.0, dhs-to). It
# carries no RCS $Id, no version macro -- the only trace of its lineage is
# three headers commenting "Initial install into CVS of dhs-0.16", against a
# directory dated 10 Jun 2013. So the RPM becomes the version of record, and
# the changelog carries the hashes, exactly as gem-vxworks-tornado22 does for
# the equally unversioned vxWorks kernel.
#
# The headers matter as much as the libraries: dhs.h is included by
# detControl.h, and without it four of hrwfs's nineteen sources do not
# compile. No RPM has ever shipped them -- the same gap the dhs migration
# found on the Linux side, where dhsClient ships libraries and no include/.

%global _binaries_in_noarch_packages_terminate_build 0
%global _build_id_links none
%global __os_install_post %{nil}
%global debug_package %{nil}
%global dhsdir /gemini/dhs/dhs

Name:           hrwfs-dhs-vxlibs
Version:        0.16
Release:        1%{?dist}
Summary:        DHS/DRAMA vxWorks client libraries and headers for the hrwfs crate
License:        Proprietary (Gemini / AAO DRAMA) -- org-internal
AutoReqProv:    no
BuildArch:      noarch

%description
The mv2700T2 DHS and DRAMA client objects the hrwfs IOC loads at boot, plus
the 44 headers hrwfs compiles against. Installed at %{dhsdir}, the path the
startup script names.

Version 0.16 is inferred, not declared: the tree carries no version marker
and the only evidence is a CVS comment in three headers. Treat the RPM, not
the directory, as the statement of what is installed.

%install
mkdir -p %{buildroot}%{dhsdir}/external/lib %{buildroot}%{dhsdir}/lib
cp -a %{trees}%{dhsdir}/external/lib/mv2700T2 %{buildroot}%{dhsdir}/external/lib/
cp -a %{trees}%{dhsdir}/lib/mv2700T2          %{buildroot}%{dhsdir}/lib/
cp -a %{trees}%{dhsdir}/include               %{buildroot}%{dhsdir}/
chown -R root:root %{buildroot}%{dhsdir}

# The five objects the startup script ld's, by name. A missing one is a crate
# that stops mid-boot, so fail here instead.
for f in external/lib/mv2700T2/libers.o external/lib/mv2700T2/libimp.o \
         external/lib/mv2700T2/libsds.o lib/mv2700T2/libgen.a \
         lib/mv2700T2/libdhs.a; do
    [ -f "%{buildroot}%{dhsdir}/$f" ] || {
        echo "ERROR: missing $f -- the crate ld's this at boot" >&2; exit 1; }
done
# And the header four hrwfs sources include via detControl.h.
[ -f "%{buildroot}%{dhsdir}/include/dhs.h" ] || {
    echo "ERROR: missing include/dhs.h -- hrwfs will not compile" >&2; exit 1; }

%files
%defattr(-,root,root,-)
%{dhsdir}

%changelog
* Mon Sep 14 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 0.16-1
- Initial packaging. The tree carries no internal version marker, so these
  hashes are the only way to tell one copy from another:
    libers.o   7590  sha256 fab652769e6d6447...
    libimp.o 244650  sha256 2c7220dd8b395834...
    libsds.o  74357  sha256 ab39cb37b69f1de7...
    libdhs.a 177014  sha256 ebe830f62270bd77...
    libgen.a  22018  sha256 4548eccbfb306659...
- Includes the 44 headers, which no RPM has ever shipped.
