[schematic2]
uniq 82
[tools]
[detail]
s -160 2208 500 0 Wavefront Sensing - Error Log Status Records
s 2512 -704 500 512 errorSad.sch
[cell use]
use esirs 1888 1223 100 0 errorLog1
xform 0 2096 1376
p 1952 1184 100 0 1 DESC:Latest error message
p 1824 960 100 0 0 FDSC:Latest error message
p 1952 1152 100 0 1 FTVL:STRING
p 1952 1088 100 0 1 PV:$(sadtop)
p 1952 1120 100 0 1 SNAM:
use esirs 1888 1735 100 0 errorLog
xform 0 2096 1888
p 1952 1696 100 0 1 DESC:Latest error message
p 1824 1472 100 0 0 FDSC:Latest error message
p 1952 1664 100 0 1 FTVL:STRING
p 1952 1600 100 0 1 PV:$(sadtop)
p 1952 1632 100 0 1 SNAM:
use esirs 1200 1223 100 0 historyLog1
xform 0 1408 1376
p 1264 1184 100 0 1 DESC:Latest log message
p 1136 960 100 0 0 FDSC:Latest message (history log)
p 1264 1152 100 0 1 FTVL:STRING
p 1264 1088 100 0 1 PV:$(sadtop)
p 1264 1120 100 0 1 SNAM:
use esirs 1184 1735 100 0 historyLog
xform 0 1392 1888
p 1248 1696 100 0 1 DESC:Latest log message
p 1120 1472 100 0 0 FDSC:Latest message (history log)
p 1248 1664 100 0 1 FTVL:STRING
p 1248 1600 100 0 1 PV:$(sadtop)
p 1248 1632 100 0 1 SNAM:
use notes 3520 -265 100 0 notes#13
xform 0 3776 -80
p 4048 -114 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3548 46 100 0 -1 COMMENT1:This schematic contains the error
p 3548 14 100 0 -1 COMMENT2:log status records.
p 3548 -48 100 0 -1 COMMENT4:NOTE: The error count records assume
p 3548 -80 100 0 -1 COMMENT5:there are between 1 and 3 processors.
p 3548 -112 100 0 -1 COMMENT6:It would be more flexible to put each
p 3548 -144 100 0 -1 COMMENT7:set of these records in a separate
p 3548 -176 100 0 -1 COMMENT8:"procSad.sch" schematic. SMB - 4/2/98.
use bd200tr -1024 -904 -100 0 frame
xform 0 1616 800
p 2608 -672 200 0 1 author:S.M.Beard
p 3120 -704 100 0 0 border:D
p 2608 -752 200 0 1 checked:B.Goodrich
p 3184 -688 200 0 -1 date:$Date: 1999-03-17 03:14:12 $
p 2592 2336 200 0 -1 id:$Id: errorLogSad.sch,v 1.1.1.1 1999-03-17 03:14:12 cboyer Exp $
p 3120 -416 200 0 -1 project:Gemini Wavefront Sensing System
p 2592 -480 200 0 -1 revision:$Revision: 1.1.1.1 $
p 3120 -544 200 0 -1 title:Error Log Status Records
[comments]
