[schematic2]
uniq 25
[tools]
[detail]
w 840 1483 100 0 n#24 egenSub.probeOffset.OUTJ 800 1472 928 1472 hwout.hwout#23.outp
w -216 395 100 0 n#21 egenSub.aoZero.OUTJ -256 384 -128 384 hwout.hwout#20.outp
w -216 1483 100 0 n#18 egenSub.ttfZero.OUTJ -256 1472 -128 1472 hwout.hwout#17.outp
s 1488 80 500 512 testWfs.sch
s -592 2192 500 0 Wavefront sensing - WFS test harness
[cell use]
use hwout 928 1431 100 0 hwout#23
xform 0 1024 1472
p 1024 1463 100 0 -1 val(outp):$(top)$(wfs)probeOffset.J PP NMS
use hwout -128 343 100 0 hwout#20
xform 0 -32 384
p -32 375 100 0 -1 val(outp):$(top)$(wfs)aoZero.J PP NMS
use hwout -128 1431 100 0 hwout#17
xform 0 -32 1472
p -32 1463 100 0 -1 val(outp):$(top)$(wfs)ttfZero.J PP NMS
use egenSub 512 1287 100 0 probeOffset
xform 0 656 1712
p 576 1248 100 0 1 DESC:Test probeOffset record
p 608 2064 100 0 1 FTA:LONG
p 592 1488 100 0 1 FTVJ:DOUBLE
p 576 1216 100 0 1 INAM:testInit
p 592 1552 100 0 1 NOJ:9
p 592 1520 100 0 1 NOVJ:9
p 576 1120 100 0 1 PV:$(top)$(test)$(wfs)
p 576 1152 100 0 1 SCAN:Passive
p 576 1184 100 0 1 SNAM:testProbeOffset
p 800 1482 75 0 -1 pproc(OUTJ):PP
use egenSub -544 199 100 0 aoZero
xform 0 -400 624
p -480 160 100 0 1 DESC:Test aoZero record
p -448 976 100 0 1 FTA:LONG
p -464 400 100 0 1 FTVJ:DOUBLE
p -480 128 100 0 1 INAM:testInit
p -464 464 100 0 1 NOJ:24
p -464 432 100 0 1 NOVJ:24
p -480 32 100 0 1 PV:$(top)$(test)$(wfs)
p -480 64 100 0 1 SCAN:Passive
p -480 96 100 0 1 SNAM:testAoZero
p -256 394 75 0 -1 pproc(OUTJ):PP
use egenSub -544 1287 100 0 ttfZero
xform 0 -400 1712
p -480 1248 100 0 1 DESC:Test ttfZero record
p -448 2064 100 0 1 FTA:LONG
p -464 1488 100 0 1 FTVJ:DOUBLE
p -480 1216 100 0 1 INAM:testInit
p -464 1552 100 0 1 NOJ:8
p -464 1520 100 0 1 NOVJ:8
p -480 1120 100 0 1 PV:$(top)$(test)$(wfs)
p -480 1152 100 0 1 SCAN:Passive
p -480 1184 100 0 1 SNAM:testTtfZero
p -256 1482 75 0 -1 pproc(OUTJ):PP
use notes 1536 263 100 0 notes#15
xform 0 1792 448
p 2064 414 100 0 0 AUTHOR:S.M.Beard
p 1564 574 100 0 -1 COMMENT1:This is the test harness for a
p 1564 542 100 0 -1 COMMENT2:particular wavefront sensor
use bc200tr -1024 -120 -100 0 frame
xform 0 656 1184
p 1552 48 100 0 -1 author:$Author: cboyer $
p 1776 32 100 0 -1 border:C
p 1552 16 100 0 1 checked:B.Goodrich
p 1776 0 100 0 -1 date:$Date: 1999-03-17 03:14:13 $
p 1552 2352 100 0 -1 id:$Id: testWfs.sch,v 1.1.1.1 1999-03-17 03:14:13 cboyer Exp $
p 1792 160 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 128 100 0 -1 revision:$Revision: 1.1.1.1 $
p 1792 96 100 0 -1 title:Diagram Title
[comments]
