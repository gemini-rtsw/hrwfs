[schematic2]
uniq 99
[tools]
[detail]
w 856 459 100 0 n#98 ecars.configC.OMSS 768 448 992 448 992 1184 1056 1184 egenSub.combApplyC.INPH
w 840 523 100 0 n#97 ecars.configC.VAL 768 512 960 512 960 1248 1056 1248 egenSub.combApplyC.INPG
w 952 1323 100 0 n#96 hrwfsSubsysActWait.hrwfsSubsysActWait#90.OMSS1 768 800 896 800 896 1312 1056 1312 egenSub.combApplyC.INPF
w 936 1387 100 0 n#95 hrwfsSubsysActWait.hrwfsSubsysActWait#90.VAL1 768 864 864 864 864 1376 1056 1376 egenSub.combApplyC.INPE
w 920 1451 100 0 n#94 hrwfsSubsysActWait.hrwfsSubsysActWait#90.OMSS 768 928 832 928 832 1440 1056 1440 egenSub.combApplyC.INPD
w 904 1515 100 0 n#93 hrwfsSubsysActWait.hrwfsSubsysActWait#90.VAL 768 992 800 992 800 1504 1056 1504 egenSub.combApplyC.INPC
w 832 1771 100 0 n#92 carID.carID#23.FLNK 784 1760 928 1760 928 960 1056 960 egenSub.combApplyC.SLNK
w 824 299 100 0 n#92 ecars.configC.FLNK 768 288 928 288 928 960 junction
w 824 747 100 0 n#92 hrwfsSubsysActWait.hrwfsSubsysActWait#90.FLNK 768 736 928 736 junction
w 320 747 100 0 n#91 inhier.RESET.P 272 736 416 736 hrwfsSubsysActWait.hrwfsSubsysActWait#90.RESET
w 848 1835 100 0 n#84 carID.carID#23.OMSS 784 1824 960 1824 960 1568 1056 1568 egenSub.combApplyC.INPB
w 864 1899 100 0 n#83 carID.carID#23.VAL 784 1888 992 1888 992 1632 1056 1632 egenSub.combApplyC.INPA
w 1448 1579 100 0 n#82 egenSub.combApplyC.OUTB 1344 1568 1600 1568 1600 1440 1664 1440 ecars.applyC.IVAL
w 1544 1387 100 0 n#81 egenSub.combApplyC.OUTA 1344 1632 1472 1632 1472 1376 1664 1376 ecars.applyC.IMSS
w 1584 1419 100 0 n#80 inhier.ICID.P 1552 1408 1664 1408 ecars.applyC.ICID
w 1416 939 100 0 n#79 egenSub.combApplyC.FLNK 1344 928 1536 928 1536 1248 1664 1248 ecars.applyC.SLNK
w 336 523 100 0 n#77 inhier.IVAL.P 272 512 448 512 ecars.configC.IVAL
w -456 907 100 0 n#59 hrwfsCarNext1.hrwfsCarNext1#65.FLNK -464 896 -400 896 junction
w -456 1323 100 0 n#59 hrwfsCarNext.hrwfsCarNext#62.FLNK -464 1312 -400 1312 junction
w -456 555 100 0 n#59 wfsCar.wfsCar#56.FLNK -464 544 -400 544 -400 1216 junction
w -396 1467 100 0 n#59 hrwfsCar.hrwfsCar#18.FLNK -464 1728 -400 1728 -400 1216 -48 1216 egenSub.combActive.SLNK
w -328 1003 100 0 n#69 hrwfsCarNext1.hrwfsCarNext1#65.CLID -464 992 -144 992 -144 1440 -48 1440 egenSub.combActive.INPH
w -344 1067 100 0 n#68 hrwfsCarNext1.hrwfsCarNext1#65.VAL -464 1056 -176 1056 -176 1504 -48 1504 egenSub.combActive.INPG
w -184 1707 100 0 n#64 hrwfsCarNext.hrwfsCarNext#62.CLID -464 1408 -272 1408 -272 1696 -48 1696 egenSub.combActive.INPD
w -200 1771 100 0 n#63 hrwfsCarNext.hrwfsCarNext#62.VAL -464 1472 -304 1472 -304 1760 -48 1760 egenSub.combActive.INPC
w -360 651 100 0 n#58 wfsCar.wfsCar#56.CLID -464 640 -208 640 -208 1568 -48 1568 egenSub.combActive.INPF
w -376 715 100 0 n#57 wfsCar.wfsCar#56.VAL -464 704 -240 704 -240 1632 -48 1632 egenSub.combActive.INPE
w 468 1467 100 0 n#48 egenSub.combActive.FLNK 240 1184 464 1184 464 1760 592 1760 carID.carID#23.SLNK
w 472 1859 100 0 n#47 egenSub.combActive.OUTB 240 1824 400 1824 400 1856 592 1856 carID.carID#23.ICID
w 392 1899 100 0 n#46 egenSub.combActive.OUTA 240 1888 592 1888 carID.carID#23.IVAL
w -280 1891 100 0 n#37 hrwfsCar.hrwfsCar#18.VAL -464 1888 -48 1888 egenSub.combActive.INPA
w -280 1827 100 0 n#36 hrwfsCar.hrwfsCar#18.CLID -464 1824 -48 1824 egenSub.combActive.INPB
s 1488 80 500 512 hrwfsCarTree.sch
s -576 2224 500 0 Gemini High Resolution Wavefront Sensor
[cell use]
use inhier 1536 1367 100 0 ICID
xform 0 1552 1408
use inhier 256 471 100 0 IVAL
xform 0 272 512
use inhier 256 695 100 0 RESET
xform 0 272 736
use hrwfsSubsysActWait 448 751 100 0 hrwfsSubsysActWait#90
xform 0 592 904
p 448 672 100 0 1 seta:prefix ag
p 448 640 100 0 1 setb:subsys hrwfs:cc:
p 448 608 100 0 1 setc:text A&G
use ecars 1664 1159 100 0 applyC
xform 0 1824 1328
p 1696 1504 100 0 -1 DESC:Top level CAR for OCS
use ecars 448 231 100 0 configC
xform 0 608 400
use egenSub 1056 871 100 0 combApplyC
xform 0 1200 1296
p 1056 1712 100 0 -1 DESC:Combine configC and activeC
p 833 645 100 0 0 FTA:LONG
p 833 645 100 0 0 FTB:STRING
p 833 613 100 0 0 FTC:LONG
p 833 581 100 0 0 FTD:STRING
p 833 549 100 0 0 FTE:LONG
p 833 485 100 0 0 FTF:STRING
p 833 485 100 0 0 FTG:LONG
p 833 453 100 0 0 FTH:STRING
p 833 645 100 0 0 FTVA:STRING
p 833 645 100 0 0 FTVB:LONG
p 1120 816 100 0 1 SNAM:hrwfsCarCombine
p 1344 1578 75 0 -1 pproc(OUTB):NPP
use egenSub -48 1127 100 0 combActive
xform 0 96 1552
p 16 1088 100 0 1 DESC:Combine CAR values
p -176 1920 100 0 1 FTA:LONG
p -176 1856 100 0 1 FTB:LONG
p -176 1792 100 0 1 FTC:LONG
p -176 1728 100 0 1 FTD:LONG
p -176 1664 100 0 1 FTE:LONG
p -176 1600 100 0 1 FTF:LONG
p -176 1536 100 0 1 FTG:LONG
p -176 1472 100 0 1 FTH:LONG
p -176 1408 100 0 1 FTI:LONG
p -176 1344 100 0 1 FTJ:LONG
p 272 1920 100 0 1 FTVA:LONG
p 272 1856 100 0 1 FTVB:LONG
p 272 1792 100 0 1 FTVC:LONG
p 16 1024 100 0 1 PV:$(top)
p 16 1056 100 0 1 SNAM:cicsCarValCombine
p -271 901 100 0 0 UFC:
use hrwfsCarNext1 -656 759 100 0 hrwfsCarNext1#65
xform 0 -560 944
use hrwfsCarNext -656 1191 100 0 hrwfsCarNext#62
xform 0 -560 1360
use wfsCar -656 471 100 0 wfsCar#56
xform 0 -560 592
p -656 416 100 0 1 set1:wfs dc:
use carID 592 1639 100 0 carID#23
xform 0 688 1792
p 592 1632 100 0 1 set1:car Top level
p 592 1600 100 0 1 set2:pv $(top)active
use hrwfsCar -656 1655 100 0 hrwfsCar#18
xform 0 -560 1776
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:SMB & CJM
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 1 checked:
p 1776 16 100 0 -1 date:$Date: 2001-06-28 12:00:30 $
p 1552 2368 100 0 -1 id:$Id: hrwfsCarTree.sch,v 1.3 2001-06-28 12:00:30 cjm Exp $
p 1792 176 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 144 100 0 -1 revision:$Revision: 1.3 $
p 1792 112 100 0 -1 title:Top level CAR combination schematic
[comments]
