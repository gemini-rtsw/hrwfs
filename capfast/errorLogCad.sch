[schematic2]
uniq 79
[tools]
[detail]
s 32 1984 500 0 Wavefront Sensing - Error Log CAD Records
s 2112 -80 500 512 errorLogCad.sch
[cell use]
use bc200tr -368 -264 -100 0 frame
xform 0 1312 1040
p 2208 -96 100 0 -1 author:S.M.Beard
p 2432 -112 100 0 -1 border:C
p 2208 -128 100 0 1 checked:A.Foster
p 2432 -144 100 0 -1 date:$Date: 1999-03-17 03:14:14 $
p 2208 2208 100 0 -1 id:$Id: errorLogCad.sch,v 1.1.1.1 1999-03-17 03:14:14 cboyer Exp $
p 2448 16 100 0 -1 project:Gemini Wavefront Sensing System
p 2208 -16 100 0 -1 revision:$Revision: 1.1.1.1 $
p 2448 -48 100 0 -1 title:Error Log CAD Records
use ecad2 1328 967 100 0 errorLogClear
xform 0 1488 1280
p 1392 928 100 0 1 DESC:Clear error counters
p 1440 1344 100 0 1 FTVA:STRING
p 1440 1280 100 0 1 FTVB:STRING
p 1392 896 100 0 1 INAM:epToVxCadInit
p 1392 832 100 0 1 PV:$(top)
p 1392 864 100 0 1 SNAM:epToVxCadExecute
use ecad2 816 967 100 0 errorLogClose
xform 0 976 1280
p 880 928 100 0 1 DESC:Close error log file
p 928 1344 100 0 1 FTVA:STRING
p 928 1280 100 0 1 FTVB:STRING
p 880 896 100 0 1 INAM:epToVxCadInit
p 880 832 100 0 1 PV:$(top)
p 880 864 100 0 1 SNAM:epToVxCadExecute
use ecad4 336 839 100 0 errorLogOpen
xform 0 496 1216
p 400 800 100 0 1 DESC:Open error log file
p 448 1344 100 0 1 FTVA:STRING
p 448 1280 100 0 1 FTVB:STRING
p 448 1216 100 0 1 FTVC:STRING
p 448 1152 100 0 1 FTVD:STRING
p 400 768 100 0 1 INAM:epToVxCadInit
p 400 704 100 0 1 PV:$(top)
p 400 736 100 0 1 SNAM:epToVxCadExecute
use notes 2272 135 100 0 notes#13
xform 0 2528 320
p 2800 286 100 0 0 AUTHOR:S.M.Beard
p 2300 446 100 0 -1 COMMENT1:This schematic contains the CAD records
p 2300 414 100 0 -1 COMMENT2:for the subset of the systemwide AGWPS
p 2300 384 100 0 -1 COMMENT3:commands devoted to error logging.
p 2300 352 100 0 -1 COMMENT4:.
p 2300 320 100 0 -1 COMMENT5:See ICD 1.6.2/1.6.3 for a description
p 2300 288 100 0 -1 COMMENT6:of what these commands do.
[comments]
