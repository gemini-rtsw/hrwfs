#!/usr/bin/csh -f
#
#	MODULE NAME:
#	wfsLogin
#
#	FILENAME:
#	wfsLogin_<SITE>
#
#	INVOCATION:
#	source wfsLogin_<SITE>
#
#	PURPOSE:
#	Sets up the environment variables needed by the Gemini A&G WFS software
#
#	DESCRIPTION:
#	This scripts sets up the environment variables pointing to the various
#	Gemini libraries on which the Gemini A&G WFS software depends. The script
#	may be executed as is or used as an example. It may eventually be replaced
#	by a standard Gemini startup script.
#
#	NOTE:
#	THIS SCRIPT IS SITE SPECIFIC. IT WILL NEED TO BE EDITED
#	FOR YOUR OWN SITE.
#
#	AUTHOR:
#	Steven M Beard
#
#	HISTORY:
#	$Id: wfsLogin_Gemini.bash,v 1.1.1.1 1999-03-17 03:14:10 cboyer Exp $
#
#	$Log: not supported by cvs2svn $
#	Revision 1.1.1.1  1999/03/05 01:21:31  cboyer
#	Initial creation of the Gemini WFS repository
#
#	Revision 1.3  1998/10/20 09:35:42  cics
#	Trivial change
#
#	Revision 1.2  1998/09/28 11:05:30  cics
#	Now depends on cFitsIo as well
#
#	Revision 1.1  1998/07/17 11:33:45  smb
#	Example login script added to system.
#
#

# Execute the Gemini EPICS startup script if it exists.

#if ( -e /gemini/epics/epics.csh ) then  
#
#    source /gemini/epics/epics.csh
#else
#    echo "*** EPICS startup script not found."
#    exit
#endif

# Set up environment variables for location of Gemini libraries if they 
# exist.
#
# NOTE: Gemini libraries at ROE are contained in two different directories.
# This situation needs tidying up. In the final gemini system everything
# will be contained within a single /gemini directory.

if [ -e $EPICS/extensions/src/gemini ] && [ -e /gemini ] ; then

    export GEMINI_LIB=$EPICS/extensions/src/gemini ;
    export GEMINI_SLALIB=$GEMINI_LIB/slalib ;
    export GEMINI_TIMELIB=$GEMINI_LIB/timelib ;
    export GEMINI_ASTLIB=/gemini/astlib ;
    export GEMINI_CFITSIO=/gemini/cfitsio ;
    export GEMINI_DHS=/gemini/dhs ;
    export DRAMA_DIR=/gemini/dhs/source/drama ;
else

    echo "*** Gemini library directories not found." ;
fi

# Define aliases for useful commands
 
# Command for printing a CAPFAST diagram

#alias schprint  "schplot -Nb '\!^'.sch | schps -wc 26 -d | lp" 

# Command for dumping an X window screen (e.g. "dm" screen) into Postscript

#alias x2ps       "xwd -nobdrs | xpr -device ps -height 7.5 -width 6.5 -compact -portrait"
