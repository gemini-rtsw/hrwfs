[schematic2]
uniq 39
[tools]
[detail]
w 1768 514 100 0 n#25 efanouts.tFan.LNK1 2992 1152 3104 1152 3104 512 480 512 480 1120 junction
w 376 1122 100 0 n#25 inhier.STOP.P 160 1120 640 1120 elongouts.tStp.SLNK
w 1282 1360 100 0 n#38 elongouts.tTog.VAL 1216 1264 1280 1264 1280 1408 1392 1408 ecalc.tInc.INPB
w 930 1344 100 0 n#37 elongouts.tGo.OUT 896 1376 928 1376 928 1264 960 1264 elongouts.tTog.SLNK
w 930 1200 100 0 n#37 elongouts.tStp.OUT 896 1088 928 1088 928 1264 junction
w 3072 1122 100 0 n#35 efanouts.tFan.LNK2 2992 1120 3200 1120 outhier.EXPIRED.p
w 2210 1224 100 0 n#33 ecalc.tInc.FLNK 2096 1440 2208 1440 2208 960 2320 960 ecalcs.tExp.SLNK
w 2146 1016 100 0 n#32 eaos.tOut.VAL 1936 672 2144 672 2144 1312 2320 1312 ecalcs.tExp.INPB
w 1688 1730 100 0 n#29 ecalc.tInc.VAL 2096 1344 2144 1344 2144 1728 1280 1728 1280 1440 1392 1440 ecalc.tInc.INPA
w 2208 1346 100 0 n#29 junction 2144 1344 2320 1344 ecalcs.tExp.INPA
w 376 1410 100 0 n#24 inhier.START.P 160 1408 640 1408 elongouts.tGo.SLNK
w 560 1154 100 0 n#19 hwin.hwin#17.in 528 1152 640 1152 elongouts.tStp.DOL
w 560 1442 100 0 n#18 hwin.hwin#16.in 528 1440 640 1440 elongouts.tGo.DOL
w 1608 706 100 0 n#10 hwin.hwin#9.in 1584 704 1680 704 eaos.tOut.DOL
w 2674 1146 100 0 n#5 ecalcs.tExp.FLNK 2608 1184 2672 1184 2672 1072 2752 1072 efanouts.tFan.SLNK
w 2686 1154 100 0 n#4 ecalcs.tExp.VAL 2608 1152 2800 1152 efanouts.tFan.SELL
s 2832 192 100 0 Gemini Telescope Control System
s 2592 2384 100 0 $Id: hrwfsTimeOut.sch,v 1.1 2001-06-24 18:55:50 gemvx Exp $
s 1344 2240 200 0 Timeout counter 
s 544 896 100 0 The toggle record is only present as it proved impossible
s 544 864 100 0 to define a user port on the calc record for field B.
s 544 832 100 0 Defining FLD4 as B resulted in being unable to write
s 544 800 100 0 anything into field B even from cau.
s 1216 2176 100 0 The counter increments once a second until the timeout
s 1216 2144 100 0 period is exceeded or the counter is stopped. If the timeout
s 1216 2112 100 0 period is exceeded then the EXPIRED output link is fired.
s 1216 2032 100 0 To use the schematic the macros timeout and top must
s 1216 2000 100 0 both be defined on a higher level schematic.
[cell use]
use elongouts 664 1032 100 0 tStp
xform 0 768 1120
p 704 1230 100 0 -1 DESC:Stop timer
p 480 1038 100 0 0 OMSL:closed_loop
p 896 1088 75 768 -1 pproc(OUT):PP
use elongouts 664 1320 100 0 tGo
xform 0 768 1408
p 704 1518 100 0 -1 DESC:Start timer
p 480 1326 100 0 0 OMSL:closed_loop
p 896 1376 75 768 -1 pproc(OUT):PP
use elongouts 984 1176 100 0 tTog
xform 0 1088 1264
p 1024 1374 100 0 -1 DESC:Toggle timer
p 800 1182 100 0 0 OMSL:supervisory
use ecalc 1416 968 100 0 tInc
xform 0 1744 1296
p 1520 1086 100 0 0 ADEL:0.000000000000000e+00
p 1715 1544 100 0 -1 CALC:B=1?A+1:0
p 1601 1494 100 0 1 DESC:Incrementing counter
p 1520 1278 100 0 0 DISS:NO_ALARM
p 1520 1310 100 0 1 DISV:1
p 1520 1214 100 0 1 EGU:none
p 1520 1374 100 0 0 EVNT:0
p 1776 1246 100 0 0 HHSV:NO_ALARM
p 1776 1310 100 0 0 HIGH:0.0000000e+00
p 1776 1374 100 0 0 HIHI:0.0000000e+00
p 1776 1438 100 0 0 HOPR:0.0000000e+00
p 1776 1214 100 0 0 HSV:NO_ALARM
p 1520 1118 100 0 0 HYST:0.000000000000000e+00
p 1776 1150 100 0 0 LLSV:NO_ALARM
p 1776 1342 100 0 0 LOLO:0.0000000e+00
p 1776 1406 100 0 0 LOPR:0.0000000e+00
p 1776 1278 100 0 0 LOW:0.0000000e+00
p 1776 1182 100 0 0 LSV:NO_ALARM
p 1520 1054 100 0 0 MDEL:0.000000000000000e+00
p 1520 1406 100 0 0 PHAS:0
p 1520 1342 100 0 0 PINI:NO
p 1520 1182 100 0 0 PREC:0
p 1520 1246 100 0 0 PRIO:LOW
p 1520 1438 100 0 1 SCAN:1 second
p 1424 1568 100 1024 -1 username(U4):FLD4
use inhier 168 1080 100 0 STOP
xform 0 160 1120
use inhier 168 1368 100 0 START
xform 0 160 1408
use hwin 360 1112 100 0 hwin#17
xform 0 432 1152
p 339 1144 100 0 -1 val(in):0
use hwin 360 1400 100 0 hwin#16
xform 0 432 1440
p 339 1432 100 0 -1 val(in):1
use hwin 1416 664 100 0 hwin#9
xform 0 1488 704
p 1395 696 100 0 -1 val(in):$(timeout)
use eaos 1704 584 100 0 tOut
xform 0 1808 672
p 1744 782 100 0 -1 DESC:Timeout value
p 1648 398 100 0 0 EGU:seconds
p 1424 654 100 0 0 OMSL:closed_loop
use outhier 3192 1080 100 0 EXPIRED
xform 0 3184 1120
use efanouts 2776 936 100 0 tFan
xform 0 2872 1088
p 2608 1054 100 0 0 SELM:Mask
use ecalcs 2344 872 100 0 tExp
xform 0 2464 1136
p 2432 1070 100 0 -1 CALC:A>B?3:0
p 2384 1406 100 0 -1 DESC:Check for expiry
use bc200tr 0 -88 -100 0 frame
xform 0 1680 1216
[comments]
