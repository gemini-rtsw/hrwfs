[schematic2]
uniq 79
[tools]
[detail]
w 1224 466 100 0 n#77 efanouts.Fan2.LNK2 1376 736 1456 736 1456 464 1040 464 1040 352 1072 352 eaos.trig.SLNK
w 1504 322 100 0 n#76 eaos.trig.OUT 1328 320 1728 320 outhier.c#70.p
w 1016 386 100 0 n#75 hwin.hwin#73.in 1008 384 1072 384 eaos.trig.DOL
w 1872 674 100 0 n#69 estringouts.null.FLNK 2016 800 2064 800 2064 672 1728 672 1728 544 1760 544 eaos.idle.SLNK
w 1480 770 100 0 n#68 efanouts.Fan2.LNK1 1376 768 1632 768 1632 784 1760 784 estringouts.null.SLNK
w 1704 578 100 0 n#67 hwin.hwin#66.in 1696 576 1760 576 eaos.idle.DOL
w 768 898 100 0 n#63 efanouts.Fan1.LNK2 1584 1344 1680 1344 1680 896 -96 896 -96 640 -32 640 eaos.subAct.SLNK
w 360 642 100 0 n#62 eaos.subAct.VAL 224 640 544 640 544 848 672 848 ecalcs.isBusy.INPA
w 312 674 100 0 n#61 eaos.subAct.FLNK 224 672 448 672 448 464 672 464 ecalcs.isBusy.SLNK
w 1096 770 100 0 n#60 ecalcs.isBusy.VAL 960 656 1056 656 1056 768 1184 768 efanouts.Fan2.SELL
w 1024 690 100 0 n#59 ecalcs.isBusy.FLNK 960 688 1136 688 efanouts.Fan2.SLNK
w -224 674 100 0 n#56 inhier.c#55.P -368 672 -32 672 eaos.subAct.DOL
w 64 2082 100 0 n#30 inhier.c#37.P -464 2080 640 2080 hrwfsTimeOut.hrwfsTimeOut#78.STOP
w 1048 1682 100 0 n#30 efanouts.Fan1.FLNK 1584 1408 1648 1408 1648 1680 496 1680 496 2080 junction
w 1544 2322 100 0 n#52 hrwfsTimeOut.hrwfsTimeOut#78.EXPIRED 1088 2272 1392 2272 1392 2320 1744 2320 estringouts.noResp.SLNK
w 1816 1874 100 0 n#35 estringouts.ScWso.FLNK 1984 1728 2032 1728 2032 1872 1648 1872 1648 2016 junction
w 1840 2130 100 0 n#35 estringouts.noResp.FLNK 2000 2336 2080 2336 2080 2128 1648 2128 1648 2016 1744 2016 eaos.errVal.SLNK
w 1682 1568 100 0 n#51 efanouts.Fan1.LNK1 1584 1376 1680 1376 1680 1712 1728 1712 estringouts.ScWso.SLNK
w 2184 1698 100 0 n#50 estringouts.ScWso.OUT 1984 1696 2432 1696 outhier.c#49.p
w 1472 1746 100 0 n#48 inhier.c#47.P 1264 1744 1728 1744 estringouts.ScWso.DOL
w 1304 1378 100 0 n#45 ecalcs.subErr.VAL 1200 1264 1264 1264 1264 1376 1392 1376 efanouts.Fan1.SELL
w 1248 1298 100 0 n#44 ecalcs.subErr.FLNK 1200 1296 1344 1296 efanouts.Fan1.SLNK
w 808 1458 100 0 n#42 ewait.ScWait.VAL 656 1360 752 1360 752 1456 912 1456 ecalcs.subErr.INPA
w 760 1074 100 0 n#41 ewait.ScWait.FLNK 656 1072 912 1072 ecalcs.subErr.SLNK
w 64 2274 100 0 n#38 inhier.START.P -464 2272 640 2272 hrwfsTimeOut.hrwfsTimeOut#78.START
w 2192 1986 100 0 n#34 eaos.errVal.OUT 2000 1984 2432 1984 outhier.c#32.p
w 2192 2306 100 0 n#33 estringouts.noResp.OUT 2000 2304 2432 2304 outhier.c#31.p
w 1680 2050 100 0 n#29 hwin.hwin#28.in 1664 2048 1744 2048 eaos.errVal.DOL
w -248 1458 100 0 n#11 inhier.c#10.P -400 1456 -48 1456 ewait.ScWait.INAN
s -576 2368 100 0 index - a number to enable unique names to be generated
s -576 2400 100 0 subcad - name of subsystem cad record
s -576 2432 100 0 subsys - prefix of the subsystem eg. ec:
s 640 2368 100 0 record and again an error is flagged.
s 640 2400 100 0 the rejection message is copied to the appropriate CAR
s 640 2432 100 0 If the subsystem does respond but rejects the command then
s 640 2464 100 0 timeout period an error is flagged. 
s 640 2496 100 0 the command is sent and if there is no response within the
s 640 2528 100 0 when the TCS sends it a command. A timer is started when
s 640 2560 100 0 This schematic handles error responses from a subsystem
s 1824 2592 100 0 $Id: hrwfsSubCadWait.sch,v 1.1 2001-06-24 19:00:50 gemvx Exp $
s 2096 400 100 0 Gemini Telescope Control System
s 2112 336 100 0 Get status from Subsystem CAD
s 2336 272 100 0 1
s 2448 272 100 0 1
s 0 976 100 0 ScWait
s -592 2528 100 0 N.B. Use of this schematic requires the following macros:
s -576 2496 100 0 text - set to name of subsystem to give informative message
s -576 2464 100 0 timeout - timeout in seconds
[cell use]
use hrwfsTimeOut 640 2023 100 0 hrwfsTimeOut#78
xform 0 864 2176
p 660 1996 100 0 1 seta:top $(top)$(index)
use eaos 1096 264 100 0 trig
xform 0 1200 352
p 1104 446 100 0 -1 DESC:Trigger timer
p 1040 78 100 0 0 EGU:none
p 816 334 100 0 0 OMSL:closed_loop
p 1312 286 100 0 -1 def(OUT):$(hrwfs)$(prefix)StartTimer.PROC
p 1328 320 75 768 -1 pproc(OUT):PP
use eaos 1784 456 100 0 idle
xform 0 1888 544
p 1792 638 100 0 -1 DESC:Write IDLE status
p 1728 270 100 0 0 EGU:none
p 1504 526 100 0 0 OMSL:closed_loop
p 2144 542 100 0 -1 def(OUT):$(hrwfs)$(prefix)CommSentC.IVAL
p 2016 512 75 768 -1 pproc(OUT):PP
use eaos -8 552 100 0 subAct
xform 0 96 640
p -64 734 100 0 -1 DESC:Pull in current subsystem activeC value
p -64 366 100 0 0 EGU:none
p -288 622 100 0 0 OMSL:closed_loop
p -336 688 100 0 -1 def(DOL):$(hrwfs)$(prefix)ActiveC
use eaos 1768 1928 100 0 errVal
xform 0 1872 2016
p 1776 2110 100 0 -1 DESC:Write error status
p 1712 1742 100 0 0 EGU:none
p 1488 1998 100 0 0 OMSL:closed_loop
p 2128 2014 100 0 -1 def(OUT):$(hrwfs)$(prefix)CommSentC.IVAL
p 2000 1984 75 768 -1 pproc(OUT):PP
use hwin 840 344 100 0 hwin#73
xform 0 912 384
p 819 376 100 0 -1 val(in):1
use hwin 1528 536 100 0 hwin#66
xform 0 1600 576
p 1507 568 100 0 -1 val(in):$(CAR_IDLE)
use hwin 1496 2008 100 0 hwin#28
xform 0 1568 2048
p 1475 2040 100 0 -1 val(in):$(CAR_ERROR)
use outhier 1720 280 100 0 c#70
xform 0 1712 320
use outhier 2424 1656 100 0 c#49
xform 0 2416 1696
use outhier 2424 1944 100 0 c#32
xform 0 2416 1984
use outhier 2424 2264 100 0 c#31
xform 0 2416 2304
use estringouts 1784 712 100 0 null
xform 0 1888 784
p 1760 878 100 0 -1 DESC:Empty message
p 1696 590 100 0 0 OMSL:supervisory
p 1696 750 100 0 0 VAL:
p 1360 830 100 0 0 def(DOL):0.0
p 2096 798 100 0 -1 def(OUT):$(hrwfs)$(prefix)CommSentC.IMSS
use estringouts 1752 1640 100 0 ScWso
xform 0 1856 1712
p 1728 1806 100 0 -1 DESC:Subcad error message
p 1664 1518 100 0 0 OMSL:closed_loop
p 1664 1678 100 0 0 VAL:OK
p 1328 1758 100 0 -1 def(DOL):$(subsys)$(subcad).MESS
p 2064 1726 100 0 -1 def(OUT):$(hrwfs)$(prefix)CommSentC.IMSS
use estringouts 1768 2248 100 0 noResp
xform 0 1872 2320
p 1776 2414 100 0 -1 DESC:No response string
p 1680 2286 100 0 0 VAL:No response from $(text)
p 2112 2334 100 0 -1 def(OUT):$(hrwfs)$(prefix)CommSentC.IMSS
use efanouts 1160 552 100 0 Fan2
xform 0 1256 704
p 992 670 100 0 0 SELM:Specified
p 1152 318 100 0 0 def(LNK2):0.0
p 1408 736 75 1280 -1 pproc(LNK2):NPP
use efanouts 1368 1160 100 0 Fan1
xform 0 1464 1312
p 1200 1278 100 0 0 SELM:Specified
use ecalcs 696 376 100 0 isBusy
xform 0 816 640
p 784 574 100 0 -1 CALC:A=2?1:2
use ecalcs 936 984 100 0 subErr
xform 0 1056 1248
p 1024 1198 100 0 -1 CALC:A<0?1:2
use inhier -360 632 100 0 c#55
xform 0 -368 672
use inhier 1272 1704 100 0 c#47
xform 0 1264 1744
use inhier -456 2040 100 0 c#37
xform 0 -464 2080
use inhier -456 2232 100 0 START
xform 0 -464 2272
use inhier -392 1416 100 0 c#10
xform 0 -400 1456
use bc200tr -752 120 -100 0 frame
xform 0 928 1424
use ewait -24 984 -100 0 ScWait
xform 0 304 1312
p 224 1566 100 0 1 CALC:A
p 625 437 100 0 0 DTYP:Soft Channel
p 336 1360 100 0 1 INAP:Yes
p 336 1328 100 0 0 INBP:No
p 336 1296 100 0 0 INCP:No
p 80 1136 100 0 1 OOPT:Every Time
p 80 1454 100 0 1 SCAN:I/O Intr
p -352 1470 100 0 -1 def(INAN):$(subsys)$(subcad).VAL
p 172 960 100 1024 0 name:$(top)$(I)
[comments]
