[schematic2]
uniq 89
[tools]
[detail]
w 1272 939 100 0 n#88 estringouts.clearError.FLNK 1440 864 1504 864 1504 928 1088 928 1088 400 1184 400 estringouts.clearError1.SLNK
w 1544 395 100 0 n#87 estringouts.clearError1.OUT 1440 384 1696 384 hwout.hwout#86.outp
w 1544 843 100 0 n#84 estringouts.clearError.OUT 1440 832 1696 832 hwout.hwout#83.outp
s -160 2208 500 0 Wavefront Sensing - Error Log Status Records
s 2512 -704 500 512 errorLogSad.sch
[cell use]
use hwout 1696 343 100 0 hwout#86
xform 0 1792 384
p 1808 384 100 0 -1 val(outp):$(sadtop)$(wfs)errorLog1 PP NMS
use estringouts 1184 327 100 0 clearError1
xform 0 1312 400
p 1248 240 100 0 1 OMSL:closed_loop
p 1248 272 100 0 1 PV:$(sadtop)$(wfs)
p 1248 208 100 0 1 VAL:0
p 1440 384 75 768 -1 pproc(OUT):PP
use hwout 1696 791 100 0 hwout#83
xform 0 1792 832
p 1792 823 100 0 -1 val(outp):$(sadtop)$(wfs)errorLog PP NMS
use estringouts 1184 775 100 0 clearError
xform 0 1312 848
p 1248 688 100 0 1 OMSL:supervisory
p 1248 720 100 0 1 PV:$(sadtop)$(wfs)
p 1440 832 75 768 -1 pproc(OUT):PP
use esirs 1888 1223 100 0 errorLog1
xform 0 2096 1376
p 1952 1184 100 0 1 DESC:Latest error message
p 1824 960 100 0 0 FDSC:Latest error message
p 1952 1152 100 0 1 FTVL:STRING
p 1952 1088 100 0 1 PV:$(sadtop)$(wfs)
p 1952 1120 100 0 1 SNAM:
use esirs 1888 1735 100 0 errorLog
xform 0 2096 1888
p 1952 1696 100 0 1 DESC:Latest error message
p 1824 1472 100 0 0 FDSC:Latest error message
p 1952 1664 100 0 1 FTVL:STRING
p 1952 1600 100 0 1 PV:$(sadtop)$(wfs)
p 1952 1632 100 0 1 SNAM:
use esirs 1184 1223 100 0 historyLog1
xform 0 1392 1376
p 1248 1184 100 0 1 DESC:Latest log message
p 1120 960 100 0 0 FDSC:Latest message (history log)
p 1248 1152 100 0 1 FTVL:STRING
p 1248 1088 100 0 1 PV:$(sadtop)$(wfs)
p 1248 1120 100 0 1 SNAM:
use esirs 1184 1735 100 0 historyLog
xform 0 1392 1888
p 1248 1696 100 0 1 DESC:Latest log message
p 1120 1472 100 0 0 FDSC:Latest message (history log)
p 1248 1664 100 0 1 FTVL:STRING
p 1248 1600 100 0 1 PV:$(sadtop)$(wfs)
p 1248 1632 100 0 1 SNAM:
use bd200tr -1024 -904 -100 0 frame
xform 0 1616 800
p 2608 -672 200 0 1 author:S.M.Beard
p 3120 -704 100 0 0 border:D
p 2608 -752 200 0 1 checked:B.Goodrich
p 3184 -688 200 0 -1 date:$Date: 2001-02-15 01:43:40 $
p 2592 2336 200 0 -1 id:$Id: errorLogSad.sch,v 1.3 2001-02-15 01:43:40 cboyer Exp $
p 3120 -416 200 0 -1 project:Gemini Wavefront Sensing System
p 2592 -480 200 0 -1 revision:$Revision: 1.3 $
p 3120 -544 200 0 -1 title:Error Log Status Records
[comments]
