[schematic2]
uniq 191
[tools]
[detail]
w 994 1675 100 0 n#137 carID.carID#165.FLNK 928 1664 1120 1664 junction
w 1554 1483 100 0 n#137 carID.carID#21.FLNK 928 2176 1120 2176 1120 1472 2048 1472 egenSub.combSystemNext1.SLNK
w 994 1163 100 0 n#137 carID.carID#173.FLNK 928 1152 1120 1152 1120 1472 junction
w 994 651 100 0 n#137 carID.carID#174.FLNK 928 640 1120 640 1120 1152 junction
w 994 139 100 0 n#137 carID.carID#187.FLNK 928 128 1120 128 1120 640 junction
w 1394 235 100 0 n#190 carID.carID#187.CLID 928 224 1920 224 1920 1568 2048 1568 egenSub.combSystemNext1.INPJ
w 642 267 100 0 n#183 elongouts.guidePut.OUT 608 256 736 256 carID.carID#187.IVAL
w 1266 331 100 0 n#183 junction 704 256 704 320 1888 320 1888 1632 2048 1632 egenSub.combSystemNext1.INPI
w 1362 747 100 0 n#189 carID.carID#174.CLID 928 736 1856 736 1856 1696 2048 1696 egenSub.combSystemNext1.INPH
w 642 779 100 0 n#178 elongouts.endVerifyPut.OUT 608 768 736 768 carID.carID#174.IVAL
w 1234 843 100 0 n#178 junction 704 768 704 832 1824 832 1824 1760 2048 1760 egenSub.combSystemNext1.INPG
w 1330 1259 100 0 n#188 carID.carID#173.CLID 928 1248 1792 1248 1792 1824 2048 1824 egenSub.combSystemNext1.INPF
w 642 1291 100 0 n#169 elongouts.verifyPut.OUT 608 1280 736 1280 carID.carID#173.IVAL
w 1202 1355 100 0 n#169 junction 704 1280 704 1344 1760 1344 1760 1888 2048 1888 egenSub.combSystemNext1.INPE
w 280 299 100 0 n#182 esirs.guiding.FLNK 192 352 256 352 256 288 352 288 elongouts.guidePut.SLNK
w 248 331 100 0 n#181 esirs.guiding.VAL 192 320 352 320 elongouts.guidePut.DOL
w 248 843 100 0 n#180 esirs.endVerifying.VAL 192 832 352 832 elongouts.endVerifyPut.DOL
w 280 811 100 0 n#179 esirs.endVerifying.FLNK 192 864 256 864 256 800 352 800 elongouts.endVerifyPut.SLNK
w 280 1323 100 0 n#168 esirs.verifying.FLNK 192 1376 256 1376 256 1312 352 1312 elongouts.verifyPut.SLNK
w 248 1355 100 0 n#167 esirs.verifying.VAL 192 1344 352 1344 elongouts.verifyPut.DOL
w 1298 1771 100 0 n#166 carID.carID#165.CLID 928 1760 1728 1760 1728 1952 2048 1952 egenSub.combSystemNext1.INPD
w 1170 1867 100 0 n#162 junction 704 1792 704 1856 1696 1856 1696 2016 2048 2016 egenSub.combSystemNext1.INPC
w 642 1803 100 0 n#162 elongouts.endObservePut.OUT 608 1792 736 1792 carID.carID#165.IVAL
w 280 1835 100 0 n#161 esirs.endObserving.FLNK 192 1888 256 1888 256 1824 352 1824 elongouts.endObservePut.SLNK
w 248 1867 100 0 n#160 esirs.endObserving.VAL 192 1856 352 1856 elongouts.endObservePut.DOL
w 1282 2283 100 0 n#133 carID.carID#21.CLID 928 2272 1696 2272 1696 2080 2048 2080 egenSub.combSystemNext1.INPB
w 642 2315 100 0 n#129 elongouts.endGuidePut.OUT 608 2304 736 2304 carID.carID#21.IVAL
w 1186 2379 100 0 n#129 junction 704 2304 704 2368 1728 2368 1728 2144 2048 2144 egenSub.combSystemNext1.INPA
w 280 2347 100 0 n#121 esirs.endGuiding.FLNK 192 2400 256 2400 256 2336 352 2336 elongouts.endGuidePut.SLNK
w 248 2379 100 0 n#120 esirs.endGuiding.VAL 192 2368 352 2368 elongouts.endGuidePut.DOL
w 2526 2179 100 0 VAL egenSub.combSystemNext1.VALA 2336 2176 2752 2176 2752 2304 3040 2304 outhier.VAL.p
w 2664 1451 100 0 FLNK egenSub.combSystemNext1.FLNK 2336 1440 3040 1440 outhier.FLNK.p
w 2552 2115 100 0 CLID egenSub.combSystemNext1.VALB 2336 2112 2816 2112 2816 1984 3040 1984 outhier.CLID.p
w 2664 2147 100 0 OVAL egenSub.combSystemNext1.OUTA 2336 2144 3040 2144 outhier.OVAL.p
s 2000 -352 500 512 hrwfsCarNext.sch
s -256 2560 500 0 Wavefront Sensing - System CAR Records
[cell use]
use carID 736 7 100 0 carID#187
xform 0 832 160
p 736 0 100 0 1 set1:car Guide
p 736 -32 100 0 1 set2:pv $(top)guide
use carID 736 519 100 0 carID#174
xform 0 832 672
p 736 512 100 0 1 set1:car EndVerify
p 736 480 100 0 1 set2:pv $(top)endVerify
use carID 736 1031 100 0 carID#173
xform 0 832 1184
p 736 1024 100 0 1 set1:car Verify
p 736 992 100 0 1 set2:pv $(top)verify
use carID 736 1543 100 0 carID#165
xform 0 832 1696
p 736 1536 100 0 1 set1:car EndObserve
p 736 1504 100 0 1 set2:pv $(top)endObserve
use carID 736 2055 100 0 carID#21
xform 0 832 2208
p 736 2048 100 0 1 set1:car EndGuide
p 736 2016 100 0 1 set2:pv $(top)endGuide
use elongouts 352 199 100 0 guidePut
xform 0 480 288
p 192 430 100 0 0 EGU:CAR state
p 416 176 100 0 1 OMSL:closed_loop
p 416 144 100 0 1 PV:$(top)
p 608 256 75 768 -1 pproc(OUT):PP
use elongouts 352 711 100 0 endVerifyPut
xform 0 480 800
p 192 942 100 0 0 EGU:CAR state
p 416 688 100 0 1 OMSL:closed_loop
p 416 656 100 0 1 PV:$(top)
p 608 768 75 768 -1 pproc(OUT):PP
use elongouts 352 1223 100 0 verifyPut
xform 0 480 1312
p 192 1454 100 0 0 EGU:CAR state
p 416 1200 100 0 1 OMSL:closed_loop
p 416 1168 100 0 1 PV:$(top)
p 608 1280 75 768 -1 pproc(OUT):PP
use elongouts 352 1735 100 0 endObservePut
xform 0 480 1824
p 192 1966 100 0 0 EGU:CAR state
p 416 1712 100 0 1 OMSL:closed_loop
p 416 1680 100 0 1 PV:$(top)
p 608 1792 75 768 -1 pproc(OUT):PP
use elongouts 352 2247 100 0 endGuidePut
xform 0 480 2336
p 192 2478 100 0 0 EGU:CAR state
p 416 2224 100 0 1 OMSL:closed_loop
p 416 2192 100 0 1 PV:$(top)
p 608 2304 75 768 -1 pproc(OUT):PP
use esirs -224 103 100 0 guiding
xform 0 -16 256
p -160 64 100 0 1 DESC:WFS system guiding status
p -128 240 100 0 1 EGU:CAR state
p -288 -160 100 0 0 FDSC:WFS sys status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -128 272 100 0 1 FTVL:LONG
p 0 208 100 0 1 HIGH:2
p 0 176 100 0 1 HIHI:3
p -128 176 100 0 1 LOLO:0
p -128 208 100 0 1 LOW:0
p -160 32 100 0 1 PV:$(top)
use esirs -224 615 100 0 endVerifying
xform 0 -16 768
p -160 576 100 0 1 DESC:WFS system end verify
p -128 752 100 0 1 EGU:CAR state
p -288 352 100 0 0 FDSC:WFS sys status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -128 784 100 0 1 FTVL:LONG
p 0 720 100 0 1 HIGH:2
p 0 688 100 0 1 HIHI:3
p -128 688 100 0 1 LOLO:0
p -128 720 100 0 1 LOW:0
p -160 544 100 0 1 PV:$(top)
use esirs -224 1127 100 0 verifying
xform 0 -16 1280
p -160 1088 100 0 1 DESC:WFS system verify
p -128 1264 100 0 1 EGU:CAR state
p -288 864 100 0 0 FDSC:WFS sys status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -128 1296 100 0 1 FTVL:LONG
p 0 1232 100 0 1 HIGH:2
p 0 1200 100 0 1 HIHI:3
p -128 1200 100 0 1 LOLO:0
p -128 1232 100 0 1 LOW:0
p -160 1056 100 0 1 PV:$(top)
use esirs -224 1639 100 0 endObserving
xform 0 -16 1792
p -160 1600 100 0 1 DESC:WFS system end observe status
p -128 1776 100 0 1 EGU:CAR state
p -288 1376 100 0 0 FDSC:WFS sys status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -128 1808 100 0 1 FTVL:LONG
p 0 1744 100 0 1 HIGH:2
p 0 1712 100 0 1 HIHI:3
p -128 1712 100 0 1 LOLO:0
p -128 1744 100 0 1 LOW:0
p -160 1568 100 0 1 PV:$(top)
use esirs -224 2151 100 0 endGuiding
xform 0 -16 2304
p -160 2112 100 0 1 DESC:WFS system guide status
p -128 2288 100 0 1 EGU:CAR state
p -288 1888 100 0 0 FDSC:WFS sys status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -128 2320 100 0 1 FTVL:LONG
p 0 2256 100 0 1 HIGH:2
p 0 2224 100 0 1 HIHI:3
p -128 2224 100 0 1 LOLO:0
p -128 2256 100 0 1 LOW:0
p -160 2080 100 0 1 PV:$(top)
use bd200tr -1504 -568 -100 0 frame
xform 0 1136 1136
p 2128 -336 200 0 -1 author:C. Boyer
p 2640 -368 100 0 0 border:D
p 2128 -416 200 0 1 checked:
p 2624 -432 200 0 -1 date:2000/01/20
p 2112 2672 200 0 -1 id:hrwfsCarNext1.sch
p 2640 -80 200 0 -1 project:Gemini Wavefront Sensing System
p 2128 -160 200 0 -1 revision:$Revision: 1.1 $
p 2640 -208 200 0 -1 title:System CAR Records
use egenSub 2048 1383 100 0 combSystemNext1
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
use notes 2880 151 100 0 notes#13
xform 0 3136 336
p 3408 302 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 2908 462 100 0 -1 COMMENT1:This schematic contains the CAR records
p 2908 430 100 0 -1 COMMENT2:for systemwide actions.
use outhier 3008 2103 100 0 OVAL
xform 0 3024 2144
use outhier 3008 1943 100 0 CLID
xform 0 3024 1984
use outhier 3008 1783 100 0 OMSS
xform 0 3024 1824
use outhier 3008 1591 100 0 OERR
xform 0 3024 1632
use outhier 3008 1399 100 0 FLNK
xform 0 3024 1440
use outhier 3008 2263 100 0 VAL
xform 0 3024 2304
[comments]
