[schematic2]
uniq 110
[tools]
[detail]
w 626 1643 100 0 n#108 hwin.hwin#107.in 608 1632 704 1632 egenSub.combSysInit.INPI
w 626 1771 100 0 n#105 hwin.hwin#106.in 608 1760 704 1760 egenSub.combSysInit.INPG
w 626 1899 100 0 n#104 hwin.hwin#103.in 608 1888 704 1888 egenSub.combSysInit.INPE
w 626 2027 100 0 n#102 hwin.hwin#101.in 608 2016 704 2016 egenSub.combSysInit.INPC
w 1212 1723 100 0 n#100 egenSub.combSysInit.FLNK 992 1440 1216 1440 1216 2016 1280 2016 carID.carID#23.SLNK
w 1064 2083 100 0 n#99 egenSub.combSysInit.OUTB 992 2080 1184 2080 1184 2112 1280 2112 carID.carID#23.ICID
w 264 2243 100 0 n#92 carID.carID#96.IVAL 32 2144 0 2144 0 2240 576 2240 576 2144 704 2144 egenSub.combSysInit.INPA
w -72 2155 100 0 n#92 elongouts.initPut.OUT -96 2144 0 2144 junction
w -424 2187 100 0 n#94 esirs.initialising.FLNK -512 2240 -448 2240 -448 2176 -352 2176 elongouts.initPut.SLNK
w -456 2219 100 0 n#93 esirs.initialising.VAL -512 2208 -352 2208 elongouts.initPut.DOL
w 376 2115 100 0 n#91 carID.carID#96.CLID 224 2112 576 2112 576 2080 704 2080 egenSub.combSysInit.INPB
w 380 1739 100 0 n#90 carID.carID#96.FLNK 224 2016 384 2016 384 1472 704 1472 egenSub.combSysInit.SLNK
w 1528 875 100 0 n#64 carID.carID#63.IVAL 1280 800 1248 800 1248 864 1856 864 1856 1888 2048 1888 egenSub.combSystem.INPE
w 1176 811 100 0 n#64 elongouts.measPut.OUT 1152 800 1248 800 junction
w 1544 2243 100 0 n#27 carID.carID#23.IVAL 1280 2144 1248 2144 1248 2240 1888 2240 1888 2144 2048 2144 egenSub.combSystem.INPA
w 1096 2147 100 0 n#27 egenSub.combSysInit.OUTA 992 2144 1248 2144 junction
w 376 1067 100 0 n#75 esirs.measuring.VAL 320 1056 480 1056 ecalcs.measCalc.INPA
w 388 875 100 0 n#74 esirs.measuring.FLNK 320 1088 384 1088 384 672 480 672 ecalcs.measCalc.SLNK
w 808 875 100 0 n#73 ecalcs.measCalc.VAL 768 864 896 864 elongouts.measPut.DOL
w 824 843 100 0 n#72 ecalcs.measCalc.FLNK 768 896 800 896 800 832 896 832 elongouts.measPut.SLNK
w 1522 683 100 0 n#109 carID.carID#63.FLNK 1472 672 1632 672 1632 1472 junction
w 1528 1579 100 0 n#109 carID.carID#21.FLNK 1472 1568 1632 1568 junction
w 1628 1739 100 0 n#109 carID.carID#23.FLNK 1472 2016 1632 2016 1632 1472 2048 1472 egenSub.combSystem.SLNK
w 1656 779 100 0 n#65 carID.carID#63.CLID 1472 768 1888 768 1888 1824 2048 1824 egenSub.combSystem.INPF
w 2526 2179 100 0 VAL egenSub.combSystem.VALA 2336 2176 2752 2176 2752 2304 3040 2304 outhier.VAL.p
w 2664 1451 100 0 FLNK egenSub.combSystem.FLNK 2336 1440 3040 1440 outhier.FLNK.p
w 2552 2115 100 0 CLID egenSub.combSystem.VALB 2336 2112 2816 2112 2816 1984 3040 1984 outhier.CLID.p
w 2664 2147 100 0 OVAL egenSub.combSystem.OUTA 2336 2144 3040 2144 outhier.OVAL.p
w 1592 1675 100 0 n#30 carID.carID#21.CLID 1472 1664 1760 1664 1760 1952 2048 1952 egenSub.combSystem.INPD
w 1464 1763 100 0 n#29 carID.carID#21.IVAL 1280 1696 1248 1696 1248 1760 1728 1760 1728 2016 2048 2016 egenSub.combSystem.INPC
w 1672 2115 100 0 n#28 carID.carID#23.CLID 1472 2112 1888 2112 1888 2080 2048 2080 egenSub.combSystem.INPB
s -256 2560 500 0 Wavefront Sensing - System CAR Records
s 2000 -352 500 512 systemCar.sch
[cell use]
use hwin 416 1591 100 0 hwin#107
xform 0 512 1632
p 419 1624 100 0 -1 val(in):$(top)hr:detInitC.IVAL
use hwin 416 1719 100 0 hwin#106
xform 0 512 1760
p 419 1752 100 0 -1 val(in):$(top)oi:detInitC.IVAL
use hwin 416 1847 100 0 hwin#103
xform 0 512 1888
p 419 1880 100 0 -1 val(in):$(top)p2:detInitC.IVAL
use hwin 416 1975 100 0 hwin#101
xform 0 512 2016
p 419 2008 100 0 -1 val(in):$(top)p1:detInitC.IVAL
use elongouts 896 743 100 0 measPut
xform 0 1024 832
p 736 974 100 0 0 EGU:CAR state
p 960 720 100 0 1 OMSL:closed_loop
p 960 688 100 0 1 PV:$(top)
p 1152 800 75 768 -1 pproc(OUT):PP
use elongouts -352 2087 100 0 initPut
xform 0 -224 2176
p -512 2318 100 0 0 EGU:CAR state
p -288 2064 100 0 1 OMSL:closed_loop
p -288 2032 100 0 1 PV:$(top)
p -96 2144 75 768 -1 pproc(OUT):PP
use esirs -96 839 100 0 measuring
xform 0 112 992
p -32 800 100 0 1 DESC:Wavefront measurement status
p -32 736 100 0 1 EGU:0/1
p -160 576 100 0 0 FDSC:Measurement status (0=IDLE;1=BUSY)
p -32 768 100 0 1 FTVL:LONG
p 128 704 100 0 1 HIGH:1
p 128 672 100 0 1 HIHI:1
p -32 672 100 0 1 LOLO:0
p -32 704 100 0 1 LOW:0
p -32 640 100 0 1 PV:$(top)
use esirs -928 1991 100 0 initialising
xform 0 -720 2144
p -864 1952 100 0 1 DESC:WFS system initialisation status
p -864 1888 100 0 1 EGU:CAR state
p -992 1728 100 0 0 FDSC:WFS sys status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -864 1920 100 0 1 FTVL:LONG
p -704 1856 100 0 1 HIGH:2
p -704 1824 100 0 1 HIHI:3
p -864 1824 100 0 1 LOLO:0
p -864 1856 100 0 1 LOW:0
p -864 1792 100 0 1 PV:$(top)
use carID 1280 551 100 0 carID#63
xform 0 1376 704
p 1280 544 100 0 1 set1:car WFS measurement
p 1280 512 100 0 1 set2:pv $(top)measure
use carID 1280 1895 100 0 carID#23
xform 0 1376 2048
p 1280 1888 100 0 1 set1:car Initialise
p 1280 1856 100 0 1 set2:pv $(top)init
use carID 1280 1447 100 0 carID#21
xform 0 1376 1600
p 1280 1440 100 0 1 set1:car Control
p 1280 1408 100 0 1 set2:pv $(top)control
use carID 32 1895 100 0 carID#96
xform 0 128 2048
p 32 1888 100 0 1 set1:car SysInitialise
p 32 1856 100 0 1 set2:pv $(top)wfsInit
use egenSub 2048 1383 100 0 combSystem
xform 0 2192 1808
p 2112 1344 100 0 1 DESC:Combine CAR values
p 1920 2176 100 0 1 FTA:LONG
p 1920 2112 100 0 1 FTB:LONG
p 1920 2048 100 0 1 FTC:LONG
p 1920 1984 100 0 1 FTD:LONG
p 1920 1920 100 0 1 FTE:LONG
p 1920 1856 100 0 1 FTF:LONG
p 1920 1792 100 0 1 FTG:LONG
p 1920 1728 100 0 1 FTH:LONG
p 1920 1664 100 0 1 FTI:LONG
p 1920 1600 100 0 1 FTJ:LONG
p 2368 2176 100 0 1 FTVA:LONG
p 2368 2112 100 0 1 FTVB:LONG
p 2368 2048 100 0 1 FTVC:LONG
p 2112 1280 100 0 1 PV:$(top)
p 2112 1312 100 0 1 SNAM:cicsCarValCombine
p 1825 1157 100 0 0 UFC:
use egenSub 704 1383 100 0 combSysInit
xform 0 848 1808
p 768 1344 100 0 1 DESC:Combine CAR values
p 576 2176 100 0 1 FTA:LONG
p 576 2112 100 0 1 FTB:LONG
p 576 2048 100 0 1 FTC:LONG
p 576 1984 100 0 1 FTD:LONG
p 576 1920 100 0 1 FTE:LONG
p 576 1856 100 0 1 FTF:LONG
p 576 1792 100 0 1 FTG:LONG
p 576 1728 100 0 1 FTH:LONG
p 576 1664 100 0 1 FTI:LONG
p 576 1600 100 0 1 FTJ:LONG
p 1024 2176 100 0 1 FTVA:LONG
p 1024 2112 100 0 1 FTVB:LONG
p 1024 2048 100 0 1 FTVC:LONG
p 768 1280 100 0 1 PV:$(top)
p 768 1312 100 0 1 SNAM:cicsCarValCombine
p 481 1157 100 0 0 UFC:
use bd200tr -1504 -568 -100 0 frame
xform 0 1136 1136
p 2128 -336 200 0 -1 author:S.M.Beard
p 2640 -368 100 0 0 border:D
p 2128 -416 200 0 1 checked:B.Goodrich
p 2624 -432 200 0 -1 date:$Date: 1999-03-17 03:14:12 $
p 2112 2672 200 0 -1 id:$Id: systemCar.sch,v 1.1.1.1 1999-03-17 03:14:12 cboyer Exp $
p 2640 -80 200 0 -1 project:Gemini Wavefront Sensing System
p 2128 -160 200 0 -1 revision:$Revision: 1.1.1.1 $
p 2640 -208 200 0 -1 title:System CAR Records
use notes -832 567 100 0 notes#87
xform 0 -576 752
p -304 718 100 0 0 AUTHOR:S M Beard
p -804 878 100 0 -1 COMMENT1:The "initialising" and "initPut" records
p -804 846 100 0 -1 COMMENT2:are responsible for maintaining the
p -804 816 100 0 -1 COMMENT3:"initC" CAR record. AGWPS tasks
p -804 784 100 0 -1 COMMENT4:cannot write directly to a CAR record,
p -804 752 100 0 -1 COMMENT5:so the detector controller uses
p -804 720 100 0 -1 COMMENT6:"initialising" as an intermediate SIR
p -804 688 100 0 -1 COMMENT7:to do it.
p -804 624 100 0 -1 COMMENT9:Ditto the measuring and measureC records.
use notes 2880 151 100 0 notes#13
xform 0 3136 336
p 3408 302 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 2908 462 100 0 -1 COMMENT1:This schematic contains the CAR records
p 2908 430 100 0 -1 COMMENT2:for systemwide actions.
use ecalcs 480 583 100 0 measCalc
xform 0 624 848
p 544 544 100 0 1 CALC:A*2
p 192 734 100 0 0 EGU:CAR state
p 544 512 100 0 1 PV:$(top)
use outhier 3008 2263 100 0 VAL
xform 0 3024 2304
use outhier 3008 1399 100 0 FLNK
xform 0 3024 1440
use outhier 3008 1591 100 0 OERR
xform 0 3024 1632
use outhier 3008 1783 100 0 OMSS
xform 0 3024 1824
use outhier 3008 1943 100 0 CLID
xform 0 3024 1984
use outhier 3008 2103 100 0 OVAL
xform 0 3024 2144
[comments]
