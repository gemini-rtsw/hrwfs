# hrwfs -- the Gemini High Resolution Wavefront Sensor IOC.
#
# Cross-compiles for vxWorks 5.4 / ppc604 using the GEM7 EPICS 3.13.4 tree and
# the Tornado 2.0.2 target headers, with ANL's Linux rebuild of the Wind River
# GNU tools. See docs/README-LINUX-REHOST.md in gmoscc for the general
# procedure and MIGRATION-PLAN.md here for what is specific to hrwfs.
#
# The payload installs to a FIXED path, /gemini/epics3.13.4/hrwfs/hrwfs, which
# is where the crate's boot parameters already point:
#
#   startup script (s): /gemini/epics3.13.4/hrwfs/hrwfs/bin/ppc604/startup
#
# That path is currently a symlink to the versioned directory of the day. The
# RPM replaces it with a real directory of the same name, so no boot parameter
# changes -- the same transition gmoscc made when it retired setgmos. rpm -q
# names what is installed and dnf downgrade is the rollback, which is what the
# symlink could never tell you.

%global _build_id_links none
%global __os_install_post %{nil}
%global debug_package %{nil}
%global _binaries_in_noarch_packages_terminate_build 0

# ---------------------------------------------------------------------------
# Support-library versions. These drive THREE things that must agree: the
# build-time -d flags, the runtime Requires, and the literal `ld <` paths in
# the generated startup scripts. Defining them once here is what stops a pin
# bump from leaving the crate loading a different version than was tested --
# for VxWorks that is a real hazard, because ld at boot means the copy on the
# file server IS the running code, not merely something linked against.
# Read from tools/linux-build/build.conf, the single source shared with
# setup.sh and the Makefile, so a local build and this one cannot disagree.
# rpmbuild runs from the repository root (build_rpm.sh does `cd /work`), the
# same assumption the git_hash macro already makes.
%define buildconf() %(. tools/linux-build/build.conf 2>/dev/null && echo $%1)
%global slalib_ver  %{buildconf SLALIB_VER}
%global timelib_ver %{buildconf TIMELIB_VER}
%global astlib_ver  %{buildconf ASTLIB_VER}
%global cfitsio_ver %{buildconf CFITSIO_VER}
%if "%{slalib_ver}" == ""
%{error:tools/linux-build/build.conf not readable -- rpmbuild must run from the repository root}
%endif

%global slalib_nvr  1.9.4-1.git4a156f2%{?dist}
%global timelib_nvr 1.8.6-1.git63b2b74%{?dist}
%global astlib_nvr  1.4-1.gitdcad7c0%{?dist}
%global cfitsio_nvr 4.1-1.git819bc30%{?dist}

%global supdir  /gemini/epics3.13.4/support
%global deploy  %{buildconf DEPLOY}

# $GIT_HASH first: build_rpm.sh computes it on the HOST and passes it in.
%define git_hash %(if [ -n "$GIT_HASH" ]; then echo "$GIT_HASH"; else git rev-parse --short HEAD 2>/dev/null || echo nogit; fi)

%define name    hrwfs
%define version 3.8.5
Name:           %{name}
Version:        %{version}
Release:        1.git%{git_hash}%{?dist}
Summary:        Gemini HRWFS IOC software (vxWorks 5.4 ppc604)
License:        Gemini Observatory (org-internal)
Source0:        %{name}-%{version}.tar.gz
AutoReqProv:    no
# The payload is ppc604 objects the host never executes; it serves them over
# NFS to a VME crate.
BuildArch:      noarch

BuildRequires:  gem-tornado20-linux = 2.0.2-1%{?dist}
BuildRequires:  gem-epics3134gem7 = 3.13.4-1%{?dist}
BuildRequires:  gem7-slalib-%{slalib_ver}-devel = %{slalib_nvr}
BuildRequires:  gem7-timelib-%{timelib_ver}-devel = %{timelib_nvr}
BuildRequires:  gem7-astlib-%{astlib_ver}-devel = %{astlib_nvr}
BuildRequires:  gem7-cfitsio-%{cfitsio_ver}-devel = %{cfitsio_nvr}
BuildRequires:  hrwfs-dhs-vxlibs
BuildRequires:  make, gcc, perl, tcsh

# Runtime deps are the OTHER /gemini trees the crate ld's at boot, not
# anything this host executes -- AutoReqProv finds none of them, because the
# consumer is a vxWorks target reading them over NFS. Listed explicitly so
# that installing hrwfs on the file server pulls everything a crate needs.
#
# Named WITHOUT an exact release, deliberately. The package name already
# encodes the library version -- gem7-slalib-V1-9-4 can only ever be V1-9-4 --
# and rpm permits one release of a given name at a time, so pinning the
# release makes two consumers built against different REBUILDS of the same
# library version mutually uninstallable:
#
#   cannot install both gem7-slalib-V1-9-4-...git4a156f2 and ...git008125e
#
# The pipeline README says exactly this: "leave runtime Requires loose ...
# pinning their runtime deps only creates conflicts when many are
# co-installed". What must be pinned is the library VERSION, and the name
# does that. BuildRequires above stays exact, where reproducibility matters
# and nothing is co-installed.
Requires:       gem7-epics-runtime = 3.13.4
Requires:       gem7-slalib-%{slalib_ver}
Requires:       gem7-timelib-%{timelib_ver}
Requires:       gem7-astlib-%{astlib_ver}
Requires:       gem7-cfitsio-%{cfitsio_ver}
Requires:       hrwfs-dhs-vxlibs
Requires:       gem-vxworks-tornado20 >= 2.0.2

%description
EPICS 3.13.4 (GEM7) control software for the Gemini High Resolution Wavefront
Sensor, cross-compiled for the ppc604 vxWorks target. Installs the deployable
IOC tree under %{deploy}: loadable objects, generated startup scripts,
databases, the DSP images and the detector parameter files.

%package devel
Summary:        Build environment for hrwfs development images
Requires:       gem-tornado20-linux, gem-epics3134gem7
Requires:       gem7-slalib-%{slalib_ver}-devel, gem7-timelib-%{timelib_ver}-devel
Requires:       gem7-astlib-%{astlib_ver}-devel, gem7-cfitsio-%{cfitsio_ver}-devel
Requires:       hrwfs-dhs-vxlibs
Requires:       make, gcc, perl, tcsh
%description devel
Pulls the pinned hrwfs build dependencies into a dev container.

%prep
%setup -q

%build
# Exactly what a developer runs. All build logic -- setup, the deploy path,
# the library versions -- lives in the repository (Makefile, setup.sh,
# build.conf), so this is the same command, producing the same files, as a
# local `make` in the same container. It used to be a dozen lines of sed after
# setup.sh, which a local build never ran; the two outputs quietly differed.
#
# APPLIC_SITE selects which site's #if (MK)/(CP) blocks compile in; the
# default comes from build.conf.
make %{?site:APPLIC_SITE=%{site}}

# The same checks a developer can run by hand after a local build.
./tools/linux-build/check-build.sh

%install
# Mirror the historical rdist payload (startup/UAE.dist): bin/<arch>, include,
# dbd, data, plus the DSP images and RELEASE.NOTES.
rm -rf $RPM_BUILD_ROOT
D=$RPM_BUILD_ROOT%{deploy}
mkdir -p $D/bin
cp -a bin/ppc604 $D/bin/
rm -f $D/bin/ppc604/Distfile
# The DSP images cannot be rebuilt on Linux -- asm56000 is a SPARC Solaris
# binary -- so they are committed and installed from the source tree.
mkdir -p $D/bin/asm56000
cp -a dspsrc/lod/*.lod $D/bin/asm56000/
cp -a include dbd data RELEASE.NOTES $D/
cp -a IMP_Startup.hrwfs $D/ 2>/dev/null || :

%files
%defattr(-,root,root,-)
%{deploy}

%files devel

%changelog
* Mon Sep 14 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 3.8.5-1
- Initial RPM packaging via the Linux cross-build. Source is SVN tag V3-8-5.
