# Linux equivalent of polaris' ~/.gem7 + base/config/epics.csh, in sh syntax.
# Source this before `gmake`: HOST_ARCH in particular must be in the
# environment, or the build resolves CONFIG_HOST_ARCH.unsupported and stops.
#
# The RPM-installed gem-epics3134gem7 package ships the same values as
# /etc/profile.d/gem7.sh; this copy is for a tree mounted at the Solaris paths.

export EPICS=${EPICS:-/usr/software/dev/packages/epics/epics3.13.4GEM7}
export EPICS_BASE=$EPICS/base
export HOST_ARCH=Linux
export WIND_BASE=/usr/software/dev/packages/vxworks/tornado2.0/ppc
export WIND_HOST_TYPE=x86-linux

export PATH=$EPICS_BASE/bin/$HOST_ARCH:$EPICS/extensions/bin/$HOST_ARCH:$WIND_BASE/host/$WIND_HOST_TYPE/bin:$PATH
export LD_LIBRARY_PATH=$EPICS_BASE/lib/$HOST_ARCH${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
