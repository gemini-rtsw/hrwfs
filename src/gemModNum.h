/*+
 * MODULE NAME:
 * gemModNum
 *
 * FILENAME:
 * gemModNum.h
 *
 * PURPOSE:
 * Defines VxWorks module numbers. It is expected this file might
 * one day be replaced by something supplied by Gemini.
 *
 *INDENT-OFF*
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/03/17 03:14:26  cboyer
 * Initial creation of the Gemini HRWFS repository
 *
 * Revision 1.5  1998/12/07 11:17:19  cics
 * Removed obsolete and unmanageable COPYRIGHT statement.
 *
 * Revision 1.4  1998/10/01 13:47:50  cics
 * M_signalProc blanked
 *
 * Revision 1.3  1998/05/11 13:41:47  smb
 * sigProcess renamed signalProc
 *
 * Revision 1.2  1997/12/15 11:57:17  smb
 * Added wfsControl module
 *
 * Revision 1.1.1.1  1997/11/28 11:46:17  anj
 * Imported using tkCVS
 *
 *INDENT-ON*
 *-
 */

#ifndef __INCgemModNumh
#define __INCgemModNumh

/*
 * module numbers - DO NOT CHANGE NUMBERS! Add or delete only!
 * These are designed to be processed using the vxWorks "makeStatTbl" utility.
 */

#define   M_sysextLib      (900 << 16)
#define   M_mpPipeDrv      (901 << 16)
#define   M_epToVxLib      (902 << 16)
#define   M_seqControl   (903 << 16)
#define   M_wfsLib      (904 << 16)
#define   M_archiveLib   (905 << 16)
#define   M_detControl   (906 << 16)
#define   M_blank         (907 << 16)      /* Blank entry. Can be reused. */
#define   M_errorLib      (908 << 16)
#define   M_sdsuLib      (909 << 16)
#define   M_errorLog      (910 << 16)

#endif /* __INCgemModNumh */

