[schematic2]
uniq 105
[tools]
[detail]
w 388 1979 100 2 n#103 hwin.hwin#98.in 384 1984 384 1984 egenSub.aoZero.INPA
w -348 1979 100 2 n#102 hwin.hwin#96.in -352 1984 -352 1984 egenSub.ttfZero.INPA
s -896 2080 100 0 port C is a fundge rotation angle (degrees)
s -896 2112 100 0 port B selects add or subtract the zeiss angle
s -896 2144 100 0 port A reads the probe angle from the Zeiss system
s 2464 -704 500 512 wfsGensub.sch
s 112 2224 500 0 Wavefront Sensing - WFS genSub records
[cell use]
use hwin 192 1943 100 0 hwin#98
xform 0 288 1984
p 176 2016 100 0 -1 val(in):ag:$(wfs)angle
use hwin -544 1943 100 0 hwin#96
xform 0 -448 1984
p -560 2016 100 0 -1 val(in):ag:$(wfs)angle
use egenSubC -352 7 100 0 aoDiag
xform 0 -208 432
p -352 -48 100 0 1 DESC:Display ao Diagnostics
p -352 -96 100 0 1 INAM:
p -640 558 100 0 0 PREC:2
p -352 -240 100 0 1 PV:$(top)$(wfs)
p -352 -208 100 0 1 SCAN:.5 second
p -352 -160 100 0 1 SNAM:showAoDiags
use egenSubC 384 7 100 0 fgDiag
xform 0 528 432
p 384 -48 100 0 1 DESC:Display ao Diagnostics
p 384 -96 100 0 1 INAM:
p 96 558 100 0 0 PREC:2
p 384 -240 100 0 1 PV:$(top)$(wfs)
p 384 -208 100 0 1 SCAN:.5 second
p 384 -160 100 0 1 SNAM:showFgDiags
use egenSub 1056 1223 100 0 probeOffset
xform 0 1200 1648
p 1120 1184 100 0 1 DESC:Probe offsets
p 1136 1440 100 0 1 FTJ:DOUBLE
p 1376 1440 100 0 0 FTVJ:DOUBLE
p 1120 1152 100 0 1 INAM:epToVxGensubInit
p 1136 1408 100 0 1 NOJ:9
p 1136 1376 100 0 1 NOVJ:9
p 768 1774 100 0 0 PREC:4
p 1120 1088 100 0 1 PV:$(top)$(wfs)
p 1120 1120 100 0 1 SNAM:epToVxGensubInput
use egenSub -352 1223 100 0 ttfZero
xform 0 -208 1648
p -288 1184 100 0 1 DESC:T-T-F rotation angle and WFS zero point
p -288 1024 100 0 1 FTA:STRING
p -288 992 100 0 1 FTB:STRING
p -288 960 100 0 1 FTC:STRING
p -272 1440 100 0 1 FTJ:DOUBLE
p -32 1440 100 0 0 FTVJ:DOUBLE
p -288 1152 100 0 1 INAM:
p -272 1408 100 0 1 NOJ:8
p -272 1376 100 0 1 NOVJ:8
p -640 1774 100 0 0 PREC:4
p -288 1088 100 0 1 PV:$(top)$(wfs)
p -288 1056 100 0 1 SCAN:.1 second
p -288 1120 100 0 1 SNAM:ttfZero
use egenSub 384 1223 100 0 aoZero
xform 0 528 1648
p 448 1184 100 0 1 DESC:aO rotation angle and WFS zero point
p 448 1024 100 0 1 FTA:STRING
p 448 992 100 0 1 FTB:STRING
p 448 960 100 0 1 FTC:STRING
p 464 1440 100 0 1 FTJ:DOUBLE
p 704 1440 100 0 0 FTVJ:DOUBLE
p 448 1152 100 0 1 INAM:
p 464 1408 100 0 1 NOJ:24
p 464 1376 100 0 1 NOVJ:24
p 96 1774 100 0 0 PREC:4
p 448 1088 100 0 1 PV:$(top)$(wfs)
p 448 1056 100 0 1 SCAN:.1 second
p 448 1120 100 0 1 SNAM:aoZero
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 1999-03-17 03:14:13 $
p 2592 2304 200 0 -1 id:$Id: wfsGensub.sch,v 1.1.1.1 1999-03-17 03:14:13 cboyer Exp $
p 3120 -432 200 0 -1 project:Gemini Wavefront Sensing System
p 2592 -528 200 0 -1 revision:$Revision: 1.1.1.1 $
p 3120 -560 200 0 -1 title:Wavefront Sensor genSub Records
use notes 3552 -281 100 0 notes#13
xform 0 3808 -96
p 4080 -130 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3580 30 100 0 -1 COMMENT1:This schematic contains the genSub records
p 3580 -2 100 0 -1 COMMENT2:for the commands connected with one
p 3580 -32 100 0 -1 COMMENT3:wavefront sensor. It may be duplicated
p 3580 -64 100 0 -1 COMMENT4:for each wavefront sensor, using the
p 3580 -96 100 0 -1 COMMENT5:wfs macro to distinguish each one.
p 3580 -160 100 0 -1 COMMENT7:See ICD 1.6.2/1.6.3 for a detailed
p 3580 -192 100 0 -1 COMMENT8:description of these commands.
[comments]
