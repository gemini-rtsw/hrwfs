[schematic2]
uniq 119
[tools]
[detail]
w -222 1355 100 0 n#116 ecad2.init.PLNK -352 1312 -256 1312 -256 1344 -128 1344 cadSequenceWfs.cadSequenceWfs#118.CLNK
w 88 1379 100 0 n#83 cadSequenceWfs.cadSequenceWfs#118.OUTA 96 1376 128 1376 hwout.hwout#113.outp
w -264 1251 100 0 n#82 ecad2.init.SPLK -352 1248 -128 1248 cadSequenceWfs.cadSequenceWfs#118.SPLK
w -264 1283 100 0 n#81 ecad2.init.STLK -352 1280 -128 1280 cadSequenceWfs.cadSequenceWfs#118.STLK
w -264 1379 100 0 n#79 ecad2.init.MLNK -352 1376 -128 1376 cadSequenceWfs.cadSequenceWfs#118.MLNK
w 1060 1643 100 0 n#77 ecad2.test.VALB 960 1504 1056 1504 1056 1792 1120 1792 edfans.testBFanout.DOL
w 996 1931 100 0 n#76 ecad2.test.VALA 960 1568 992 1568 992 2304 1120 2304 edfans.testAFanout.DOL
w 1412 1675 100 0 n#75 edfans.testBFanout.FLNK 1376 1920 1408 1920 1408 1440 1120 1440 1120 1312 1184 1312 cadFanout.cadFanout#31.PLNK
w 1084 2123 100 0 n#74 edfans.testAFanout.FLNK 1376 2432 1408 2432 1408 2496 1088 2496 1088 1760 1120 1760 edfans.testBFanout.SLNK
w 1020 1787 100 0 n#73 ecad2.test.PLNK 960 1312 1024 1312 1024 2272 1120 2272 edfans.testAFanout.SLNK
w 1368 1827 100 0 n#71 edfans.testBFanout.OUTA 1376 1824 1408 1824 hwout.hwout#66.outp
w 1368 2339 100 0 n#59 edfans.testAFanout.OUTA 1376 2336 1408 2336 hwout.hwout#60.outp
w 1368 1379 100 0 n#38 cadFanout.cadFanout#31.OUTA 1376 1376 1408 1376 hwout.hwout#37.outp
w 1048 1251 100 0 n#36 ecad2.test.SPLK 960 1248 1184 1248 cadFanout.cadFanout#31.SPLK
w 1048 1283 100 0 n#35 ecad2.test.STLK 960 1280 1184 1280 cadFanout.cadFanout#31.STLK
w 1048 1347 100 0 n#33 ecad2.test.CLNK 960 1344 1184 1344 cadFanout.cadFanout#31.CLNK
w 1048 1379 100 0 n#32 ecad2.test.MLNK 960 1376 1184 1376 cadFanout.cadFanout#31.MLNK
s 2240 -336 500 512 hrwfsSequenceCad.sch
s -768 2560 500 0 Wavefromt Sensing System - Sequence CAD Records
[cell use]
use cadSequenceWfs -128 1127 100 0 cadSequenceWfs#118
xform 0 -16 1280
p -128 1120 100 0 1 set1:cad init
p -128 1088 100 0 1 set2:mech wfs
use notes 3456 55 100 0 notes#13
xform 0 3712 240
p 3984 206 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3484 366 100 0 -1 COMMENT1:This schematic contains the CAD records
p 3484 334 100 0 -1 COMMENT2:for AGWPS sequence commands.
p 3484 304 100 0 -1 COMMENT3:.
p 3484 272 100 0 -1 COMMENT4:See ICD 1.6.2/1.6.3 for a description
p 3484 240 100 0 -1 COMMENT5:of what these commands do.
p 3484 208 100 0 -1 COMMENT6:.
use notes -224 1447 100 0 notes#117
xform 0 32 1632
p 304 1598 100 0 0 AUTHOR:S.M.Beard
p -196 1758 100 0 -1 COMMENT1:NOTE: PLNK is connected to CLNK
p -196 1726 100 0 -1 COMMENT2:because when the init command executes
p -196 1696 100 0 -1 COMMENT3:a PRESET, the desire is for the individual
p -196 1664 100 0 -1 COMMENT4:Detector Controller detInit commands
p -196 1632 100 0 -1 COMMENT5:to load their default parameters.
use hwout 1408 1783 100 0 hwout#66
xform 0 1504 1824
p 1504 1815 100 0 -1 val(outp):$(top)dc:detTest.B NPP NMS
use hwout 1408 2295 100 0 hwout#60
xform 0 1504 2336
p 1504 2327 100 0 -1 val(outp):$(top)dc:detTest.A NPP NMS
use hwout 1408 1335 100 0 hwout#37
xform 0 1504 1376
p 1504 1367 100 0 -1 val(outp):$(top)dc:detTest.DIR PP NMS
use hwout 128 1335 100 0 hwout#113
xform 0 224 1376
p 224 1367 100 0 -1 val(outp):$(top)dc:detInit.DIR PP NMS
use cadFanout 1184 1127 100 0 cadFanout#31
xform 0 1280 1280
p 1184 1120 100 0 1 set1:cad test
p 1184 1088 100 0 1 set2:mech wfs
use edfans 1120 2055 100 0 testAFanout
xform 0 1248 2272
p 1184 2016 100 768 1 OMSL:closed_loop
p 1184 1984 100 0 1 PV:$(top)
use edfans 1120 1543 100 0 testBFanout
xform 0 1248 1760
p 1184 1504 100 768 1 OMSL:closed_loop
p 1184 1472 100 0 1 PV:$(top)
use ecad2 -672 1191 100 0 init
xform 0 -512 1504
p -608 1152 100 0 1 DESC:Initialise
p -560 1568 100 0 0 FTVA:STRING
p -560 1504 100 0 0 FTVB:STRING
p -608 1120 100 0 1 INAM:epToVxCadInit
p -608 1056 100 0 1 PV:$(top)
p -608 1088 100 0 1 SNAM:epToVxCadExecute
use ecad2 640 1191 100 0 test
xform 0 800 1504
p 704 1152 100 0 1 DESC:Self test
p 752 1568 100 0 1 FTVA:LONG
p 752 1504 100 0 1 FTVB:LONG
p 704 1120 100 0 1 INAM:
p 704 1056 100 0 1 PV:$(top)
p 704 1088 100 0 1 SNAM:epToVxCadCopy
use ecad2 -640 199 100 0 reboot
xform 0 -480 512
p -576 160 100 0 1 DESC:Reboot system
p -528 576 100 0 1 FTVA:STRING
p -528 512 100 0 1 FTVB:STRING
p -576 128 100 0 1 INAM:epToVxCadInit
p -576 64 100 0 1 PV:$(top)
p -576 96 100 0 1 SNAM:epToVxCadExecute
use ecad2 640 199 100 0 simulate
xform 0 800 512
p 704 160 100 0 1 DESC:Set simulation mode
p 752 576 100 0 1 FTVA:STRING
p 752 512 100 0 1 FTVB:STRING
p 704 128 100 0 1 INAM:epToVxCadInit
p 704 64 100 0 1 PV:$(top)
p 704 96 100 0 1 SNAM:epToVxCadExecute
use ecad2 1920 199 100 0 debug
xform 0 2080 512
p 1984 160 100 0 1 DESC:Set debugging mode
p 2032 576 100 0 1 FTVA:STRING
p 2032 512 100 0 1 FTVB:STRING
p 1984 128 100 0 1 INAM:epToVxCadInit
p 1984 64 100 0 1 PV:$(top)
p 1984 96 100 0 1 SNAM:epToVxCadExecute
use ecad2 1920 1191 100 0 park
xform 0 2080 1504
p 1984 1152 100 0 1 DESC:Prepare for shutdown
p 2032 1568 100 0 0 FTVA:STRING
p 2032 1504 100 0 0 FTVB:STRING
p 1984 1120 100 0 1 INAM:epToVxCadInit
p 1984 1056 100 0 1 PV:$(top)
p 1984 1088 100 0 1 SNAM:epToVxCadExecute
use bd200tr -1136 -584 -100 0 frame
xform 0 1504 1120
p 2496 -352 200 0 1 author:S.M.Beard
p 3008 -384 100 0 0 border:D
p 2496 -432 200 0 1 checked:A.Foster
p 2992 -448 200 0 -1 date:$Date: 1999-11-23 03:34:48 $
p 2480 2656 200 0 -1 id:$Id: hrwfsSequenceCad.sch,v 1.2 1999-11-23 03:34:48 cboyer Exp $
p 3008 -96 200 0 -1 project:Gemini Wavefront Sensing System
p 2496 -176 200 0 -1 revision:$Revision: 1.2 $
p 3008 -224 200 0 -1 title:Sequence CAD Records
[comments]
