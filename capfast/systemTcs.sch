[schematic2]
uniq 56
[tools]
[detail]
w 1512 1099 100 0 n#54 hwin.hwin#55.in 1440 1088 1632 1088 egenSub.updateAstCtx.INPG
w 1512 1163 100 0 n#53 hwin.hwin#52.in 1440 1152 1632 1152 egenSub.updateAstCtx.INPF
w 1512 1227 100 0 n#50 hwin.hwin#51.in 1440 1216 1632 1216 egenSub.updateAstCtx.INPE
w 1512 1291 100 0 n#48 hwin.hwin#49.in 1440 1280 1632 1280 egenSub.updateAstCtx.INPD
w 1512 1355 100 0 n#47 hwin.hwin#46.in 1440 1344 1632 1344 egenSub.updateAstCtx.INPC
w 1512 1419 100 0 n#45 hwin.hwin#44.in 1440 1408 1632 1408 egenSub.updateAstCtx.INPB
w 1512 1483 100 0 n#43 hwin.hwin#42.in 1440 1472 1632 1472 egenSub.updateAstCtx.INPA
w 600 1307 100 0 n#37 ecalcs.presCalc.FLNK 496 1296 752 1296 elongins.present.SLNK
w 648 1339 100 0 n#33 junction 592 1328 752 1328 elongins.present.INP
w 328 1563 100 0 n#33 ecalcs.presCalc.VAL 496 1264 592 1264 592 1552 112 1552 112 1456 208 1456 ecalcs.presCalc.INPA
s -592 2528 500 0 Wavefront Sensing - System TCS Interface Records
s 2240 -336 500 512 systemTcs.sch
s -64 704 200 0 Records used by TCS time system
s -48 1632 200 0 Records monitored by TCS
s 1344 1632 200 0 Update WCS info from TCS
[cell use]
use hwin 1248 1047 100 0 hwin#55
xform 0 1344 1088
p 1251 1080 100 0 -1 val(in):tcs:sad:sourceAEpoch.VAL
use hwin 1248 1111 100 0 hwin#52
xform 0 1344 1152
p 1251 1144 100 0 -1 val(in):tcs:sad:sourceADec.VAL
use hwin 1248 1175 100 0 hwin#51
xform 0 1344 1216
p 1251 1208 100 0 -1 val(in):tcs:sad:sourceARA.VAL
use hwin 1248 1431 100 0 hwin#42
xform 0 1344 1472
p 1251 1464 100 0 -1 val(in):tcs:ak:astCtx.VALA
use hwin 1248 1367 100 0 hwin#44
xform 0 1344 1408
p 1251 1400 100 0 -1 val(in):tcs:sad:sourceAInputFrame.VAL
use hwin 1248 1303 100 0 hwin#46
xform 0 1344 1344
p 1251 1336 100 0 -1 val(in):tcs:sad:sourceAEquinox.VAL
use hwin 1248 1239 100 0 hwin#49
xform 0 1344 1280
p 1251 1272 100 0 -1 val(in):tcs:sad:sourceAWavelength.VAL
use egenSub 1632 711 100 0 updateAstCtx
xform 0 1776 1136
p 1728 1472 100 0 1 FTA:DOUBLE
p 1728 1408 100 0 1 FTB:STRING
p 1728 1344 100 0 1 FTC:STRING
p 1728 1280 100 0 1 FTD:DOUBLE
p 1728 1216 100 0 1 FTE:DOUBLE
p 1728 1152 100 0 1 FTF:DOUBLE
p 1728 1088 100 0 1 FTG:STRING
p 1936 1424 100 0 0 FTVB:DOUBLE
p 1696 640 100 0 1 INAM:
p 1728 1504 100 0 1 NOA:39
p 1728 1440 100 0 1 NOB:1
p 1728 1376 100 0 1 NOC:1
p 1728 1312 100 0 1 NOD:1
p 1728 1248 100 0 1 NOE:1
p 1728 1184 100 0 1 NOF:1
p 1728 1120 100 0 1 NOG:1
p 1409 133 100 0 0 NOVA:39
p 1696 576 100 0 1 PV:$(top)
p 1696 672 100 0 1 SCAN:10 second
p 1696 608 100 0 1 SNAM:wfsUpdateAstCtx
use ebos 128 -121 100 0 intSimulate
xform 0 256 -32
p 192 -160 100 0 1 DESC:TCS time system simulation mode
p 192 -192 100 0 1 ONAM:simulating
p 432 -192 100 0 1 OSV:MINOR
p 192 -256 100 0 1 PV:$(top)TIME:
p 192 -224 100 0 1 ZNAM:notSimulating
use estringins 128 151 100 0 logrecord
xform 0 256 224
p 192 112 100 0 1 DESC:TCS time log record
p 192 80 100 0 1 PV:$(top)
use eaos 128 503 100 0 health
xform 0 256 592
p 192 464 100 0 1 DESC:TCS time system health
p 96 318 100 0 0 EGU:health
p 368 368 100 0 1 HHSV:MAJOR
p 192 400 100 0 1 HIGH:100.0
p 192 368 100 0 1 HIHI:200.0
p 368 400 100 0 1 HSV:MINOR
p 192 432 100 0 1 PV:$(top)TIME:
use elongins 752 1223 100 0 present
xform 0 880 1296
p 816 1184 100 0 1 DESC:Present counter
p 496 1294 100 0 0 EGU:counts
p 816 1152 100 0 1 PV:$(top)
use ecalcs 208 983 100 0 presCalc
xform 0 352 1248
p 272 944 100 0 1 CALC:A+1
p 272 880 100 0 1 PV:$(top)
p 272 912 100 0 1 SCAN:1 second
use bd200tr -1136 -584 -100 0 frame
xform 0 1504 1120
p 2496 -352 200 0 1 author:S.M.Beard
p 3008 -384 100 0 0 border:D
p 2496 -432 200 0 1 checked:B.Goodrich
p 2992 -448 200 0 -1 date:$Date: 1999-03-17 03:14:13 $
p 2480 2656 200 0 -1 id:$Id: systemTcs.sch,v 1.1.1.1 1999-03-17 03:14:13 cboyer Exp $
p 3008 -96 200 0 -1 project:Gemini Wavefront Sensing System
p 2496 -176 200 0 -1 revision:$Revision: 1.1.1.1 $
p 3008 -224 200 0 -1 title:System TCS Interface Database
use notes 3456 55 100 0 notes#13
xform 0 3712 240
p 3984 206 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3484 366 100 0 -1 COMMENT1:This schematic contains the EPICS records
p 3484 334 100 0 -1 COMMENT2:which the TCS uses for sensing the AGWPS
p 3484 304 100 0 -1 COMMENT3:status.
p 3484 272 100 0 -1 COMMENT4:.
p 3484 240 100 0 -1 COMMENT5:See ICD 1.1.11/1.6 for a description.
p 3484 208 100 0 -1 COMMENT6:of this interface.
[comments]
