[schematic2]
uniq 79
[tools]
[detail]
w 740 347 100 2 n#78 hwout.hwout#77.outp 736 352 736 352 efanouts.testFan.LNK1
w 856 331 100 0 n#54 efanouts.testFan.LNK2 736 320 1024 320 1024 576 928 576 928 832 junction
w 344 299 100 0 n#74 carID.carID#65.FLNK 288 288 448 288 448 272 496 272 efanouts.testFan.SLNK
w 708 1067 100 0 n#54 efanouts.initFan.LNK2 640 1312 704 1312 704 832 junction
w 600 835 100 0 n#54 carID.carID#28.FLNK 288 864 288 832 960 832 egenSub.combWfs.SLNK
w 568 491 100 0 n#70 carID.carID#65.CLID 288 384 320 384 320 480 864 480 864 1184 960 1184 egenSub.combWfs.INPF
w 24 427 100 0 n#68 elongouts.testPut.OUT 0 416 96 416 carID.carID#65.IVAL
w 424 523 100 0 n#68 junction 64 416 64 512 832 512 832 1248 960 1248 egenSub.combWfs.INPE
w -360 491 100 0 n#63 esirs.testing.VAL -416 480 -256 480 elongouts.testPut.DOL
w -328 459 100 0 n#62 esirs.testing.FLNK -416 512 -352 512 -352 448 -256 448 elongouts.testPut.SLNK
w 708 1499 100 0 n#61 efanouts.initFan.LNK1 640 1344 704 1344 704 1664 736 1664 hwout.hwout#60.outp
w 324 1315 100 0 n#59 carID.carID#21.FLNK 288 1376 320 1376 320 1264 400 1264 efanouts.initFan.SLNK
w 416 1571 100 0 n#58 carID.carID#21.IVAL 96 1504 64 1504 64 1568 816 1568 816 1504 960 1504 egenSub.combWfs.INPA
w 8 1515 100 0 n#58 elongouts.initPut.OUT 0 1504 64 1504 junction
w -360 1579 100 0 n#50 esirs.initialising.VAL -416 1568 -256 1568 elongouts.initPut.DOL
w -328 1547 100 0 n#49 esirs.initialising.FLNK -416 1600 -352 1600 -352 1536 -256 1536 elongouts.initPut.SLNK
w 1796 1195 100 0 OERR carID.carID#43.OERR 1664 1408 1792 1408 1792 992 2112 992 outhier.OERR.p
w 1944 1195 100 0 OMSS carID.carID#43.OMSS 1664 1440 1824 1440 1824 1184 2112 1184 outhier.OMSS.p
w 1412 1083 100 0 n#46 egenSub.combWfs.FLNK 1248 800 1408 800 1408 1376 1472 1376 carID.carID#43.SLNK
w 1304 1451 100 0 n#45 egenSub.combWfs.OUTB 1248 1440 1408 1440 1408 1472 1472 1472 carID.carID#43.ICID
w 1336 1515 100 0 VAL egenSub.combWfs.OUTA 1248 1504 1472 1504 carID.carID#43.IVAL
w 1806 1763 100 0 VAL junction 1408 1504 1408 1760 2112 1760 outhier.VAL.p
w 1800 1379 100 0 CLID carID.carID#43.CLID 1664 1472 1856 1472 1856 1376 2112 1376 outhier.CLID.p
w 1656 803 100 0 FLNK carID.carID#43.FLNK 1664 1376 1760 1376 1760 800 2112 800 outhier.FLNK.p
w 520 963 100 0 n#55 carID.carID#28.CLID 288 960 800 960 800 1312 960 1312 egenSub.combWfs.INPD
w 392 1059 100 0 n#56 carID.carID#28.IVAL 96 992 64 992 64 1056 768 1056 768 1376 960 1376 egenSub.combWfs.INPC
w 600 1443 100 0 n#57 carID.carID#21.CLID 288 1472 288 1440 960 1440 egenSub.combWfs.INPB
s 1488 80 500 512 wfsCar.sch
s -720 2160 500 0 Wavefront Sensing - Wavefront Sensor CAR Records
[cell use]
use hwout 736 311 100 0 hwout#77
xform 0 832 352
p 832 343 100 0 -1 val(outp):$(top)combSysTest.VAL PP NMS
use efanouts 496 160 100 0 testFan
xform 0 616 288
p 560 96 100 0 1 PV:$(top)$(wfs)
p 560 128 100 0 1 SELM:All
p 768 352 75 1280 -1 pproc(LNK1):PP
p 768 320 75 1280 -1 pproc(LNK2):PP
use esirs -832 1351 100 0 initialising
xform 0 -624 1504
p -768 1312 100 0 1 DESC:WFS controller initialisation status
p -768 1248 100 0 1 EGU:CAR state
p -896 1088 100 0 0 FDSC:WFS ctrlr status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -768 1280 100 0 1 FTVL:LONG
p -608 1216 100 0 1 HIGH:2
p -608 1184 100 0 1 HIHI:3
p -768 1184 100 0 1 LOLO:0
p -768 1216 100 0 1 LOW:0
p -768 1152 100 0 1 PV:$(top)$(wfs)
use esirs -832 263 100 0 testing
xform 0 -624 416
p -768 224 100 0 1 DESC:WFS controller test status
p -768 160 100 0 1 EGU:CAR state
p -896 0 100 0 0 FDSC:WFS ctrlr status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -768 192 100 0 1 FTVL:LONG
p -608 128 100 0 1 HIGH:2
p -608 96 100 0 1 HIHI:3
p -768 96 100 0 1 LOLO:0
p -768 128 100 0 1 LOW:0
p -768 64 100 0 1 PV:$(top)$(wfs)
use elongouts -256 1447 100 0 initPut
xform 0 -128 1536
p -416 1678 100 0 0 EGU:CAR state
p -192 1424 100 0 1 OMSL:closed_loop
p -192 1392 100 0 1 PV:$(top)$(wfs)
p 0 1504 75 768 -1 pproc(OUT):PP
use elongouts -256 359 100 0 testPut
xform 0 -128 448
p -416 590 100 0 0 EGU:CAR state
p -192 336 100 0 1 OMSL:closed_loop
p -192 304 100 0 1 PV:$(top)$(wfs)
p 0 416 75 768 -1 pproc(OUT):PP
use carID 1472 1255 100 0 carID#43
xform 0 1568 1408
p 1472 1248 100 0 1 set1:car Detector
p 1472 1216 100 0 1 set2:pv $(top)$(wfs)det
use carID 96 1255 100 0 carID#21
xform 0 192 1408
p 96 1248 100 0 1 set1:car DetInitialise
p 96 1216 100 0 1 set2:pv $(top)$(wfs)detInit
use carID 96 743 100 0 carID#28
xform 0 192 896
p 96 736 100 0 1 set1:car Observation
p 96 704 100 0 1 set2:pv $(top)observe
use carID 96 167 100 0 carID#65
xform 0 192 320
p 96 160 100 0 1 set1:car DetTesting
p 96 128 100 0 1 set2:pv $(top)$(wfs)detTest
use hwout 736 1623 100 0 hwout#60
xform 0 832 1664
p 832 1655 100 0 -1 val(outp):$(top)combSysInit.VAL PP NMS
use efanouts 400 1152 100 0 initFan
xform 0 520 1280
p 464 1088 100 0 1 PV:$(top)$(wfs)
p 464 1120 100 0 1 SELM:All
p 672 1344 75 1280 -1 pproc(LNK1):PP
p 672 1312 75 1280 -1 pproc(LNK2):PP
use egenSub 960 743 100 0 combWfs
xform 0 1104 1168
p 1024 704 100 0 1 DESC:Combine CAR values
p 832 1536 100 0 1 FTA:LONG
p 832 1472 100 0 1 FTB:LONG
p 832 1408 100 0 1 FTC:LONG
p 832 1344 100 0 1 FTD:LONG
p 832 1280 100 0 1 FTE:LONG
p 832 1216 100 0 1 FTF:LONG
p 832 1152 100 0 1 FTG:LONG
p 832 1088 100 0 1 FTH:LONG
p 832 1024 100 0 1 FTI:LONG
p 832 960 100 0 1 FTJ:LONG
p 1280 1536 100 0 1 FTVA:LONG
p 1280 1472 100 0 1 FTVB:LONG
p 1280 1408 100 0 1 FTVC:LONG
p 1024 640 100 0 1 PV:$(top)$(wfs)
p 1024 672 100 0 1 SNAM:cicsCarValCombine
p 737 517 100 0 0 UFC:
use outhier 2080 1719 100 0 VAL
xform 0 2096 1760
use outhier 2080 1527 100 0 OVAL
xform 0 2096 1568
use outhier 2080 1335 100 0 CLID
xform 0 2096 1376
use outhier 2080 1143 100 0 OMSS
xform 0 2096 1184
use outhier 2080 951 100 0 OERR
xform 0 2096 992
use outhier 2080 759 100 0 FLNK
xform 0 2096 800
use notes 1536 279 100 0 notes#13
xform 0 1792 464
p 2064 430 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 1564 590 100 0 -1 COMMENT1:This schematic contains the CAR records
p 1564 558 100 0 -1 COMMENT2:for the actions specific to one
p 1564 528 100 0 -1 COMMENT3:wavefront sensor. The schematic may be
p 1564 496 100 0 -1 COMMENT4:duplicated, with a separate instance
p 1564 464 100 0 -1 COMMENT5:for each wavefront sensor; each one
p 1564 432 100 0 -1 COMMENT6:distinguished by the wfs macro.
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 1 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 2000-02-03 01:18:01 $
p 1552 2368 100 0 -1 id:$Id: wfsCar.sch,v 1.2 2000-02-03 01:18:01 cboyer Exp $
p 1792 176 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 144 100 0 -1 revision:$Revision: 1.2 $
p 1792 112 100 0 -1 title:Wavefront Sensor CAR Records
[comments]
