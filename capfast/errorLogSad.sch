[schematic2]
uniq 82
[tools]
[detail]
s 2512 -704 500 512 errorLogSad.sch
s -160 2208 500 0 Wavefront Sensing - Error Log Status Records
[cell use]
use esirs 1184 1735 100 0 historyLog
xform 0 1392 1888
p 1248 1696 100 0 1 DESC:Latest log message
p 1120 1472 100 0 0 FDSC:Latest message (history log)
p 1248 1664 100 0 1 FTVL:STRING
p 1248 1600 100 0 1 PV:$(sadtop)$(wfs)
p 1248 1632 100 0 1 SNAM:
use esirs 1200 1223 100 0 historyLog1
xform 0 1408 1376
p 1264 1184 100 0 1 DESC:Latest log message
p 1136 960 100 0 0 FDSC:Latest message (history log)
p 1264 1152 100 0 1 FTVL:STRING
p 1264 1088 100 0 1 PV:$(sadtop)$(wfs)
p 1264 1120 100 0 1 SNAM:
use esirs 1888 1735 100 0 errorLog
xform 0 2096 1888
p 1952 1696 100 0 1 DESC:Latest error message
p 1824 1472 100 0 0 FDSC:Latest error message
p 1952 1664 100 0 1 FTVL:STRING
p 1952 1600 100 0 1 PV:$(sadtop)$(wfs)
p 1952 1632 100 0 1 SNAM:
use esirs 1888 1223 100 0 errorLog1
xform 0 2096 1376
p 1952 1184 100 0 1 DESC:Latest error message
p 1824 960 100 0 0 FDSC:Latest error message
p 1952 1152 100 0 1 FTVL:STRING
p 1952 1088 100 0 1 PV:$(sadtop)$(wfs)
p 1952 1120 100 0 1 SNAM:
use bd200tr -1024 -904 -100 0 frame
xform 0 1616 800
p 2608 -672 200 0 1 author:S.M.Beard
p 3120 -704 100 0 0 border:D
p 2608 -752 200 0 1 checked:B.Goodrich
p 3184 -688 200 0 -1 date:$Date: 2000-03-13 20:46:37 $
p 2592 2336 200 0 -1 id:$Id: errorLogSad.sch,v 1.2 2000-03-13 20:46:37 cboyer Exp $
p 3120 -416 200 0 -1 project:Gemini Wavefront Sensing System
p 2592 -480 200 0 -1 revision:$Revision: 1.2 $
p 3120 -544 200 0 -1 title:Error Log Status Records
[comments]
