# EPICS 3.13.4 GEM7 *target* binaries at the path the crates load them from.
#
# Not to be confused with gem-epics3134gem7, which installs the BUILD tree
# under /usr/software/dev/packages/epics/... -- host tools, UAE config, rules,
# and the prebuilt ppc604 record objects gemini.Support is assembled from.
# That one is what gmake needs. This one is what the IOC needs: at boot the
# crate does
#
#   ld < /gemini/external/GEM7/base/bin/ppc604/iocCore
#   ld < /gemini/external/GEM7/base/bin/ppc604/seq
#   ld < /gemini/external/GEM7/extensions/bin/ppc604/pvload
#
# and nothing packaged them. They exist on the file server only because it is
# a copy of an older export, so a clean install of hrwfs and its other
# dependencies would produce a crate that mounts /gemini, cd's correctly, and
# then dies on the first ld. Same gap gem86-epics-runtime closed for GMOS.

%global _binaries_in_noarch_packages_terminate_build 0
%global _build_id_links none
%global __os_install_post %{nil}
%global debug_package %{nil}
%global tarch ppc604

Name:           gem7-epics-runtime
Version:        3.13.4
Release:        1%{?dist}
Summary:        GEM7 EPICS target binaries served to vxWorks crates
License:        EPICS Open License / Proprietary Gemini additions (org-internal)
AutoReqProv:    no
# noarch: the payload is ppc604 objects the HOST never executes -- it only
# serves them over NFS to the crate. Without this, rpmbuild stamps the
# builder's arch and the package refuses to install on the boot server.
BuildArch:      noarch

%description
The EPICS 3.13.4 GEM7 base and extensions ppc604 binaries (iocCore, seq,
pvload and companions) at /gemini/external/GEM7, the path vxWorks IOCs load
them from over NFS. Host-side build tooling is in gem-epics3134gem7.

%install
mkdir -p %{buildroot}/gemini/external/GEM7/base/bin \
         %{buildroot}/gemini/external/GEM7/extensions/bin
cp -a %{trees}/gemini/external/GEM7/base/bin/%{tarch} \
      %{buildroot}/gemini/external/GEM7/base/bin/
cp -a %{trees}/gemini/external/GEM7/extensions/bin/%{tarch} \
      %{buildroot}/gemini/external/GEM7/extensions/bin/
chown -R root:root %{buildroot}/gemini/external/GEM7

# Fail the build rather than ship a package that boots a crate into nothing:
# these three are named literally by hrwfs's startup scripts.
for f in base/bin/%{tarch}/iocCore base/bin/%{tarch}/seq \
         extensions/bin/%{tarch}/pvload; do
    [ -f "%{buildroot}/gemini/external/GEM7/$f" ] || {
        echo "ERROR: missing $f -- crates load this at boot" >&2; exit 1; }
done

%files
%defattr(-,root,root,-)
/gemini/external/GEM7

%changelog
* Mon Sep 14 2026 Hawi Stecher <hawi.stecher@noirlab.edu> - 3.13.4-1
- Initial packaging: the GEM7 EPICS target binaries were never packaged, only
  present as a copy of an older export.
