[schematic2]
uniq 204
[tools]
[detail]
w 420 843 100 2 n#203 hwin.hwin#202.in 416 848 416 848 egenSub.combDatum.INPC
w 420 2331 100 2 n#201 hwin.hwin#200.in 416 2336 416 2336 egenSub.combPark.INPC
w 866 2187 100 0 n#197 egenSub.combPark.FLNK 704 1760 800 1760 800 2176 992 2176 carID.carID#186.SLNK
w 866 2283 100 0 n#196 egenSub.combPark.OUTB 704 2400 800 2400 800 2272 992 2272 carID.carID#186.ICID
w 1282 2379 100 0 n#194 junction 960 2304 960 2368 1664 2368 1664 2144 2048 2144 egenSub.combSystemNext.INPA
w 882 2315 100 0 n#194 egenSub.combPark.OUTA 704 2464 832 2464 832 2304 992 2304 carID.carID#186.IVAL
w 1266 667 100 0 n#137 carID.carID#170.FLNK 1184 656 1408 656 1408 2176 1184 2176 carID.carID#186.FLNK
w 1698 1483 100 0 n#137 junction 1408 1472 2048 1472 egenSub.combSystemNext.SLNK
w 1378 2283 100 0 n#195 carID.carID#186.CLID 1184 2272 1632 2272 1632 2080 2048 2080 egenSub.combSystemNext.INPB
w 420 2267 100 2 n#193 hwin.hwin#192.in 416 2272 416 2272 egenSub.combPark.INPD
w 34 2187 100 0 n#190 carID.carID#21.FLNK -96 2176 224 2176 224 1792 416 1792 egenSub.combPark.SLNK
w 66 1867 100 0 n#190 ecalcouts.parkWait.FLNK -32 1856 224 1856 junction
w -382 2315 100 0 n#182 elongouts.parkPut.OUT -416 2304 -288 2304 carID.carID#21.IVAL
w -142 2379 100 0 n#182 junction -320 2304 -320 2368 96 2368 96 2464 416 2464 egenSub.combPark.INPA
w 242 2411 100 0 n#189 carID.carID#21.CLID -96 2272 128 2272 128 2400 416 2400 egenSub.combPark.INPB
w 1410 763 100 0 n#188 carID.carID#170.CLID 1184 752 1696 752 1696 1952 2048 1952 egenSub.combSystemNext.INPD
w -142 219 100 0 n#164 ecalcouts.datumWait.FLNK -544 -144 -384 -144 -384 208 160 208 160 304 junction
w 162 315 100 0 n#164 carID.carID#114.FLNK -96 656 -32 656 -32 304 416 304 egenSub.combDatum.SLNK
w 1202 987 100 0 n#171 junction 800 976 1664 976 1664 2016 2048 2016 egenSub.combSystemNext.INPC
w 866 795 100 0 n#171 egenSub.combDatum.OUTA 704 976 800 976 800 784 992 784 carID.carID#170.IVAL
w 866 667 100 0 n#173 egenSub.combDatum.FLNK 704 272 800 272 800 656 992 656 carID.carID#170.SLNK
w 850 763 100 0 n#172 egenSub.combDatum.OUTB 704 912 768 912 768 752 992 752 carID.carID#170.ICID
w 420 779 100 2 n#169 hwin.hwin#168.in 416 784 416 784 egenSub.combDatum.INPD
w 162 923 100 0 n#165 carID.carID#114.CLID -96 752 -32 752 -32 912 416 912 egenSub.combDatum.INPB
w 18 987 100 0 n#160 junction -320 784 -320 976 416 976 egenSub.combDatum.INPA
w -382 795 100 0 n#160 elongouts.datumPut.OUT -416 784 -288 784 carID.carID#114.IVAL
w -822 2411 100 0 n#121 esirs.parking.FLNK -832 2400 -752 2400 -752 2336 -672 2336 elongouts.parkPut.SLNK
w -782 2379 100 0 n#120 esirs.parking.VAL -832 2368 -672 2368 elongouts.parkPut.DOL
w -744 827 100 0 n#112 esirs.datuming.FLNK -832 880 -768 880 -768 816 -672 816 elongouts.datumPut.SLNK
w -776 859 100 0 n#111 esirs.datuming.VAL -832 848 -672 848 elongouts.datumPut.DOL
w 2526 2179 100 0 VAL egenSub.combSystemNext.VALA 2336 2176 2752 2176 2752 2304 3040 2304 outhier.VAL.p
w 2664 1451 100 0 FLNK egenSub.combSystemNext.FLNK 2336 1440 3040 1440 outhier.FLNK.p
w 2552 2115 100 0 CLID egenSub.combSystemNext.VALB 2336 2112 2816 2112 2816 1984 3040 1984 outhier.CLID.p
w 2664 2147 100 0 OVAL egenSub.combSystemNext.OUTA 2336 2144 3040 2144 outhier.OVAL.p
s -256 2560 500 0 Wavefront Sensing - System CAR Records
s 2000 -352 500 512 hrwfsCarNext.sch
[cell use]
use hwin 224 807 100 0 hwin#202
xform 0 320 848
p 227 840 100 0 -1 val(in):$(agtop)cc:datumC.VAL
use hwin 224 2295 100 0 hwin#200
xform 0 320 2336
p 227 2328 100 0 -1 val(in):$(agtop)cc:parkC.VAL
use ecalcouts -352 1671 100 0 parkWait
xform 0 -192 1792
p -280 1704 100 0 -1 CALC:A
p -288 1600 100 0 1 OOPT:On Change
p -288 1568 100 0 1 PV:$(top)
p -288 1632 100 0 1 SCAN:Passive
p -816 1856 100 0 1 def(INPA):$(agtop)cc:parkC.VAL
p -288 1672 100 0 0 name:$(top)$(I)
p -400 1864 75 0 -1 pproc(INPA):CPP
use ecalcouts -864 -329 100 0 datumWait
xform 0 -704 -208
p -792 -296 100 0 -1 CALC:A
p -800 -352 100 0 1 OOPT:On Change
p -800 -416 100 0 1 PV:$(top)
p -800 -384 100 0 1 SCAN:Passive
p -1296 -144 100 0 1 def(INPA):$(agtop)cc:datumC.VAL
p -800 -328 100 0 0 name:$(top)$(I)
p -912 -136 75 0 -1 pproc(INPA):CPP
use hwin 224 743 100 0 hwin#168
xform 0 320 784
p 227 776 100 0 -1 val(in):$(agtop)cc:datumC.CLID
use hwin 224 2231 100 0 hwin#192
xform 0 320 2272
p 227 2264 100 0 -1 val(in):$(agtop)cc:parkC.CLID
use carID 992 535 100 0 carID#170
xform 0 1088 688
p 992 528 100 0 1 set1:car Datum
p 992 496 100 0 1 set2:pv $(top)datum
use carID -288 535 100 0 carID#114
xform 0 -192 688
p -288 528 100 0 1 set1:car sysDatum
p -288 496 100 0 1 set2:pv $(top)sysDatum
use carID -288 2055 100 0 carID#21
xform 0 -192 2208
p -288 2048 100 0 1 set1:car sysPark
p -288 2016 100 0 1 set2:pv $(top)sysPark
use carID 992 2055 100 0 carID#186
xform 0 1088 2208
p 992 2048 100 0 1 set1:car Park
p 992 2016 100 0 1 set2:pv $(top)park
use egenSub 2048 1383 100 0 combSystemNext
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
use egenSub 416 215 100 0 combDatum
xform 0 560 640
p 480 176 100 0 1 DESC:Combine CAR values
p 288 1008 100 0 1 FTA:LONG
p 288 944 100 0 1 FTB:LONG
p 288 880 100 0 1 FTC:LONG
p 288 816 100 0 1 FTD:LONG
p 288 752 100 0 1 FTE:LONG
p 288 688 100 0 1 FTF:LONG
p 288 624 100 0 1 FTG:LONG
p 288 560 100 0 1 FTH:LONG
p 288 496 100 0 1 FTI:LONG
p 288 432 100 0 1 FTJ:LONG
p 736 1008 100 0 1 FTVA:LONG
p 736 944 100 0 1 FTVB:LONG
p 736 880 100 0 1 FTVC:LONG
p 480 112 100 0 1 PV:$(top)
p 480 144 100 0 1 SNAM:cicsCarValCombine
p 193 -11 100 0 0 UFC:
use egenSub 416 1703 100 0 combPark
xform 0 560 2128
p 480 1664 100 0 1 DESC:Combine CAR values
p 288 2496 100 0 1 FTA:LONG
p 288 2432 100 0 1 FTB:LONG
p 288 2368 100 0 1 FTC:LONG
p 288 2304 100 0 1 FTD:LONG
p 288 2240 100 0 1 FTE:LONG
p 288 2176 100 0 1 FTF:LONG
p 288 2112 100 0 1 FTG:LONG
p 288 2048 100 0 1 FTH:LONG
p 288 1984 100 0 1 FTI:LONG
p 288 1920 100 0 1 FTJ:LONG
p 736 2496 100 0 1 FTVA:LONG
p 736 2432 100 0 1 FTVB:LONG
p 736 2368 100 0 1 FTVC:LONG
p 480 1600 100 0 1 PV:$(top)
p 480 1632 100 0 1 SNAM:cicsCarValCombine
p 193 1477 100 0 0 UFC:
use elongouts -672 727 100 0 datumPut
xform 0 -544 816
p -832 958 100 0 0 EGU:CAR state
p -608 704 100 0 1 OMSL:closed_loop
p -608 672 100 0 1 PV:$(top)
p -416 784 75 768 -1 pproc(OUT):PP
use elongouts -672 2247 100 0 parkPut
xform 0 -544 2336
p -832 2478 100 0 0 EGU:CAR state
p -608 2224 100 0 1 OMSL:closed_loop
p -608 2192 100 0 1 PV:$(top)
p -416 2304 75 768 -1 pproc(OUT):PP
use esirs -1248 631 100 0 datuming
xform 0 -1040 784
p -1184 592 100 0 1 DESC:WFS system datum status
p -1152 768 100 0 1 EGU:CAR state
p -1312 368 100 0 0 FDSC:WFS sys status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -1152 800 100 0 1 FTVL:LONG
p -1152 704 100 0 1 HIGH:2
p -1056 704 100 0 1 HIHI:3
p -1056 736 100 0 1 LOLO:0
p -1152 736 100 0 1 LOW:0
p -1184 560 100 0 1 PV:$(top)
use esirs -1248 2151 100 0 parking
xform 0 -1040 2304
p -1184 2112 100 0 1 DESC:WFS system park status
p -1152 2288 100 0 1 EGU:CAR state
p -1312 1888 100 0 0 FDSC:WFS sys status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -1152 2320 100 0 1 FTVL:LONG
p -1024 2256 100 0 1 HIGH:2
p -1024 2224 100 0 1 HIHI:3
p -1152 2224 100 0 1 LOLO:0
p -1152 2256 100 0 1 LOW:0
p -1184 2080 100 0 1 PV:$(top)
use bd200tr -1504 -568 -100 0 frame
xform 0 1136 1136
p 2128 -336 200 0 -1 author:C. Boyer
p 2640 -368 100 0 0 border:D
p 2128 -416 200 0 1 checked:
p 2624 -432 200 0 -1 date:2000/01/19
p 2112 2672 200 0 -1 id:hrwfsCarNext.sch
p 2640 -80 200 0 -1 project:Gemini Wavefront Sensing System
p 2128 -160 200 0 -1 revision:$Revision: 1.2 $
p 2640 -208 200 0 -1 title:System CAR Records
use notes 2880 151 100 0 notes#13
xform 0 3136 336
p 3408 302 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 2908 462 100 0 -1 COMMENT1:This schematic contains the CAR records
p 2908 430 100 0 -1 COMMENT2:for systemwide actions.
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
