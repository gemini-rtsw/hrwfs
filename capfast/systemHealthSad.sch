[schematic2]
uniq 80
[tools]
[detail]
w 1476 1019 100 0 n#73 esirs.controlHealth.FLNK 1376 1376 1472 1376 1472 672 1952 672 egenSub.combHlt.SLNK
w 1720 -117 100 0 n#73 ewait.waitHealth.FLNK 1664 -128 1824 -128 1824 672 junction
w 1816 1099 100 0 n#79 ewait.waitHealth.VAL 1664 160 1728 160 1728 1088 1952 1088 egenSub.combHlt.INPE
w 1528 1323 100 0 n#75 esirs.controlHealth.OMSS 1376 1312 1728 1312 1728 1280 1952 1280 egenSub.combHlt.INPB
w 1640 1355 100 0 n#74 esirs.controlHealth.VAL 1376 1344 1952 1344 egenSub.combHlt.INPA
w 2468 923 100 0 n#54 egenSub.combHlt.FLNK 2240 640 2464 640 2464 1216 2560 1216 esirs.health.SLNK
w 2328 1291 100 0 n#53 egenSub.combHlt.OUTB 2240 1280 2464 1280 2464 1344 2560 1344 esirs.health.IMSS
w 2376 1387 100 0 n#52 egenSub.combHlt.VALA 2240 1376 2560 1376 esirs.health.INP
s -496 2224 500 0 Wavefront Sensing - System Health Status Records
s 2512 -704 500 512 systemHealthSad.sch
s 1760 1248 100 2048 ---->
s 1760 1184 100 2048 ---->
s 1536 1216 100 0 Detector controller HEALTH
[cell use]
use ewait 960 -217 100 0 waitHealth
xform 0 1312 112
p 1283 360 100 0 1 CALC:A
p 1072 320 100 0 1 DESC:Monitor component controller health
p 1088 -64 100 0 1 OOPT:On Change
p 1344 64 100 0 1 PV:$(sadtop)
p 1088 254 100 0 1 SCAN:I/O Intr
p 1088 -160 100 0 1 def(INAN):$(agtop)cc:health.VAL 
use notes 3520 -265 100 0 notes#13
xform 0 3776 -80
p 4048 -114 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3548 46 100 0 -1 COMMENT1:This schematic contains the systemwide
p 3548 14 100 0 -1 COMMENT2:health status records.
use esirs 960 1127 100 0 controlHealth
xform 0 1168 1280
p 1056 1088 100 0 1 DESC:WFS controller health
p 911 704 100 0 0 EGU: 
p 1056 1056 100 0 1 FTVL:STRING
p 1056 992 100 0 1 PV:$(sadtop)
p 1056 1024 100 0 1 SNAM:
use esirs 2560 1127 100 0 health
xform 0 2768 1280
p 2624 1088 100 0 1 DESC:Overall system health
p 2496 864 100 0 0 FDSC:Overall system health
p 2624 1056 100 0 1 FTVL:STRING
p 2624 992 100 0 1 PV:$(sadtop)
p 2624 1024 100 0 1 SNAM:
use egenSub 1952 583 100 0 combHlt
xform 0 2096 1008
p 2016 544 100 0 1 DESC:Combine health values
p 1776 1376 100 0 1 FTA:STRING
p 1776 1312 100 0 1 FTB:STRING
p 1776 1248 100 0 1 FTC:STRING
p 1776 1184 100 0 1 FTD:STRING
p 1776 1120 100 0 1 FTE:STRING
p 1776 1056 100 0 1 FTF:STRING
p 1776 992 100 0 1 FTG:STRING
p 1776 928 100 0 1 FTH:STRING
p 1776 864 100 0 1 FTI:STRING
p 1776 800 100 0 1 FTJ:STRING
p 2304 1376 100 0 1 FTVA:STRING
p 2304 1312 100 0 1 FTVB:STRING
p 2304 1248 100 0 1 FTVC:LONG
p 2016 480 100 0 1 PV:$(sadtop)
p 2016 512 100 0 1 SNAM:cicsHealthCombine
p 2016 448 100 0 1 def(INPF):$(agtop)cc:health.OMSS
use bd200tr -1024 -904 -100 0 frame
xform 0 1616 800
p 2608 -672 200 0 1 author:S.M.Beard
p 3120 -704 100 0 0 border:D
p 2608 -752 200 0 1 checked:B.Goodrich
p 3184 -688 200 0 -1 date:$Date: 2000-02-03 01:17:55 $
p 2592 2336 200 0 -1 id:$Id: systemHealthSad.sch,v 1.2 2000-02-03 01:17:55 cboyer Exp $
p 3120 -416 200 0 -1 project:Gemini Wavefront Sensing System
p 2592 -480 200 0 -1 revision:$Revision: 1.2 $
p 3120 -544 200 0 -1 title:System Health Status Records
[comments]
