[schematic2]
uniq 135
[tools]
[detail]
w 1420 747 100 0 n#69 ecalcs.counter.FLNK 1360 800 1424 800 1424 704 1488 704 esirs.heartBeat.SLNK
w 1416 867 100 0 n#68 junction 1392 864 1488 864 esirs.heartBeat.INP
w 1160 1035 100 0 n#68 ecalcs.counter.VAL 1360 768 1392 768 1392 1024 976 1024 976 960 1072 960 ecalcs.counter.INPA
w -40 331 100 0 n#63 esirs.observing.FLNK -128 384 -64 384 -64 320 32 320 elongouts.obsPut.SLNK
w -72 363 100 0 n#62 esirs.observing.VAL -128 352 32 352 elongouts.obsPut.DOL
w 280 299 100 0 n#61 elongouts.obsPut.OUT 288 288 320 288 hwout.hwout#60.outp
w 296 715 100 0 n#32 estringouts.pushVal.OUT 288 704 352 704 hwout.hwout#37.outp
w 296 939 100 0 n#31 estringouts.pushOmss.OUT 288 928 352 928 hwout.hwout#36.outp
w -110 883 100 0 n#30 esirs.health.OMSS -128 880 -32 880 -32 976 32 976 estringouts.pushOmss.DOL
w -78 955 100 0 n#29 esirs.health.FLNK -128 944 32 944 estringouts.pushOmss.SLNK
w -92 827 100 0 n#28 esirs.health.VAL -128 912 -96 912 -96 752 32 752 estringouts.pushVal.DOL
w 114 1051 100 0 n#27 estringouts.pushOmss.FLNK 288 960 352 960 352 1040 -64 1040 -64 720 32 720 estringouts.pushVal.SLNK
s 128 2176 500 0 Wavefront Sensing - WFS Status Records
s 2464 -704 500 512 wfsSad.sch
s 352 528 100 0 added after $(hindex).
s 352 560 100 0 properly, so "PP MS" has to be
s 352 592 100 0 translate the PP property of OUT
s 352 624 100 0 BUG WORK AROUND: e2sr does not
s -80 1232 100 0 These "stringout" records update the "genSub" record combHlt,
s -80 1200 100 0 which determines the overall health of the instrument.
s -80 1168 100 0 They are necessary to convert the value fields of the SIR record
s -80 1136 100 0 into link fields, which can use a channel access put.
s -80 1104 100 0 Variables "mindex" and "hindex" direct the output to the
s -80 1072 100 0 approriate "genSub" record fields for this component.
s 416 112 100 0 included in the string.
s 416 144 100 0 properly, so "PP MS" has to be
s 416 176 100 0 translate the PP property of OUT
s 416 208 100 0 BUG WORK AROUND: e2sr does not
[cell use]
use hwout 320 247 100 0 hwout#60
xform 0 416 288
p 416 279 100 0 -1 val(outp):$(top)$(wfs)observeC.IVAL PP NMS
use hwout 352 887 100 0 hwout#36
xform 0 448 928
p 448 919 100 0 -1 val(outp):$(mindex)
use hwout 352 663 100 0 hwout#37
xform 0 448 704
p 448 695 100 0 -1 val(outp):$(hindex) PP MS
use estringouts 32 871 100 0 pushOmss
xform 0 160 944
p 96 848 100 0 1 OMSL:closed_loop
p 96 816 100 0 1 PV:$(sadtop)$(wfs)
p 32 976 75 1280 -1 palrm(DOL):MS
p 320 928 75 768 -1 palrm(OUT):MS
p 288 928 75 768 -1 pproc(OUT):NPP
use estringouts 32 647 100 0 pushVal
xform 0 160 720
p 96 624 100 0 1 OMSL:closed_loop
p 96 592 100 0 1 PV:$(sadtop)$(wfs)
p 32 752 75 1280 -1 palrm(DOL):MS
p 320 704 75 768 -1 palrm(OUT):MS
p 288 704 75 768 -1 pproc(OUT):PP
use esirs 672 1703 100 0 version
xform 0 880 1856
p 736 1664 100 0 1 DESC:Software version
p 608 1440 100 0 0 FDSC:Software version
p 736 1632 100 0 1 FTVL:STRING
p 736 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1488 615 100 0 heartBeat
xform 0 1696 768
p 1552 576 100 0 1 DESC:Heart beat
p 1552 544 100 0 1 FTVL:LONG
p 1552 512 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 1703 100 0 name
xform 0 -336 1856
p -480 1664 100 0 1 DESC:WFS $(wfs) name
p -608 1440 100 0 0 FDSC:WFS $(wfs) name
p -480 1632 100 0 1 FTVL:STRING
p -480 1600 100 0 1 PV:$(sadtop)$(wfs)
p -480 1568 100 0 1 VAL:$(wfs) WFS
use esirs 64 1703 100 0 state
xform 0 272 1856
p 128 1664 100 0 1 DESC:WFS $(wfs) state
p 0 1440 100 0 0 FDSC:WFS $(wfs) state
p 128 1632 100 0 1 FTVL:STRING
p 128 1600 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 695 100 0 health
xform 0 -336 848
p -480 656 100 0 1 DESC:WFS $(wfs) health
p -608 432 100 0 0 FDSC:WFS $(wfs) health
p -480 624 100 0 1 FTVL:STRING
p -480 592 100 0 1 PV:$(sadtop)$(wfs)
use esirs -544 135 100 0 observing
xform 0 -336 288
p -480 96 100 0 1 DESC:WFS observation status
p -480 32 100 0 1 EGU:CAR state
p -608 -128 100 0 0 FDSC:WFS obs status (0=IDLE;1=PAUSED;2=BUSY;3=ERR)
p -480 64 100 0 1 FTVL:LONG
p -320 0 100 0 1 HIGH:2
p -320 -32 100 0 1 HIHI:3
p -480 -32 100 0 1 LOLO:0
p -480 0 100 0 1 LOW:0
p -480 -64 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1280 1703 100 0 testResults
xform 0 1488 1856
p 1344 1664 100 0 1 DESC:Test results
p 1216 1504 100 0 0 EGU:units
p 1216 1440 100 0 0 FDSC:Test results
p 1344 1632 100 0 1 FTVL:STRING
p 1344 1600 100 0 1 PV:$(sadtop)$(wfs)
use wfsDataSad 2960 423 100 0 wfsDataSad#102
xform 0 3056 544
use wfsDetSad 2512 423 100 0 wfsDetSad#101
xform 0 2608 544
use wfsTimSad 3408 423 100 0 wfsTimSad#100
xform 0 3504 544
use ecalcs 1072 487 100 0 counter
xform 0 1216 752
p 1136 448 100 0 1 CALC:A+1
p 1136 384 100 0 1 PV:$(sadtop)$(wfs)
p 1136 416 100 0 1 SCAN:1 second
use elongouts 32 231 100 0 obsPut
xform 0 160 320
p -128 462 100 0 0 EGU:CAR state
p 96 208 100 0 1 OMSL:closed_loop
p 96 176 100 0 1 PV:$(sadtop)$(wfs)
p 288 288 75 768 -1 pproc(OUT):PP
use notes 2816 -153 100 0 notes#13
xform 0 3072 32
p 3344 -2 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 2844 158 100 0 -1 COMMENT1:This schematic contains the Status records
p 2844 126 100 0 -1 COMMENT2:connected with one wavefront sensor.
p 2844 96 100 0 -1 COMMENT3:It may be duplicated for each wavefront
p 2844 64 100 0 -1 COMMENT4:sensor, using the wfs macro to distinguish
p 2844 32 100 0 -1 COMMENT5:each one.
use notes -560 -473 100 0 notes#49
xform 0 -304 -288
p -32 -322 100 0 0 AUTHOR:S M Beard
p -532 -162 100 0 -1 COMMENT1:The "observing" and "obsPut" records
p -532 -194 100 0 -1 COMMENT2:are responsible for maintaining the
p -532 -224 100 0 -1 COMMENT3:"observeC" CAR record. AGWPS tasks
p -532 -256 100 0 -1 COMMENT4:cannot write directly to a CAR record,
p -532 -288 100 0 -1 COMMENT5:so the detector controller uses
p -532 -320 100 0 -1 COMMENT6:"observing" as an intermediate SIR
p -532 -352 100 0 -1 COMMENT7:to do it.
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 1999-03-17 03:14:11 $
p 2576 2320 200 0 -1 id:$Id: wfsSad.sch,v 1.1.1.1 1999-03-17 03:14:11 cboyer Exp $
p 3120 -432 200 0 -1 project:Gemini Wavefront Sensing System
p 2608 -496 200 0 -1 revision:$Revision: 1.1.1.1 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Status Records
[comments]
