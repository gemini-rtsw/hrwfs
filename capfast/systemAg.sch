[schematic2]
uniq 61
[tools]
[detail]
w 8 1451 100 0 n#53 hwin.hwin#52.in -64 1440 128 1440 egenSub.updateAg.INPF
w 8 1515 100 0 n#50 hwin.hwin#51.in -64 1504 128 1504 egenSub.updateAg.INPE
w 8 1579 100 0 n#48 hwin.hwin#49.in -64 1568 128 1568 egenSub.updateAg.INPD
w 8 1643 100 0 n#47 hwin.hwin#46.in -64 1632 128 1632 egenSub.updateAg.INPC
w 8 1707 100 0 n#45 hwin.hwin#44.in -64 1696 128 1696 egenSub.updateAg.INPB
w 8 1771 100 0 n#43 hwin.hwin#42.in -64 1760 128 1760 egenSub.updateAg.INPA
s -256 1920 200 0 Update CC info from AG database
s 2240 -336 500 512 systemAg.sch
s -592 2528 500 0 Wavefront Sensing - System AG Interface Records
[cell use]
use hwin -256 1527 100 0 hwin#49
xform 0 -160 1568
p -253 1560 100 0 -1 val(in):hrwfs:fldstopName.VAL
use hwin -256 1591 100 0 hwin#46
xform 0 -160 1632
p -253 1624 100 0 -1 val(in):hrwfs:lensName.VAL
use hwin -256 1655 100 0 hwin#44
xform 0 -160 1696
p -253 1688 100 0 -1 val(in):hrwfs:ndfilterName.VAL
use hwin -256 1719 100 0 hwin#42
xform 0 -160 1760
p -253 1752 100 0 -1 val(in):hrwfs:clfilterName.VAL
use hwin -256 1463 100 0 hwin#51
xform 0 -160 1504
p -253 1496 100 0 -1 val(in):hrwfs:calName.VAL
use hwin -256 1399 100 0 hwin#52
xform 0 -160 1440
p -253 1432 100 0 -1 val(in):hrwfs:focusPosA.VALA
use egenSub 128 999 100 0 updateAg
xform 0 272 1424
p 224 1760 100 0 1 FTA:STRING
p 224 1696 100 0 1 FTB:STRING
p 224 1632 100 0 1 FTC:STRING
p 224 1568 100 0 1 FTD:STRING
p 224 1504 100 0 1 FTE:STRING
p 224 1440 100 0 1 FTF:DOUBLE
p 224 1376 100 0 0 FTG:DOUBLE
p -95 581 100 0 0 FTH:DOUBLE
p -95 549 100 0 0 FTI:DOUBLE
p 432 1712 100 0 0 FTVB:DOUBLE
p 192 928 100 0 1 INAM:
p 224 1792 100 0 1 NOA:1
p 224 1728 100 0 1 NOB:1
p 224 1664 100 0 1 NOC:1
p 224 1600 100 0 1 NOD:1
p 224 1536 100 0 1 NOE:1
p 224 1472 100 0 1 NOF:1
p 224 1408 100 0 0 NOG:1
p -95 421 100 0 0 NOVA:1
p 192 864 100 0 1 PV:$(top)
p 192 960 100 0 1 SCAN:5 second
p 192 896 100 0 1 SNAM:wfsUpdateAg
use bd200tr -1136 -584 -100 0 frame
xform 0 1504 1120
p 2496 -352 200 0 1 author:C. Boyer
p 3008 -384 100 0 0 border:D
p 2496 -432 200 0 1 checked:
p 2992 -448 200 0 -1 date:
p 2480 2656 200 0 -1 id:
p 3008 -96 200 0 -1 project:Gemini Wavefront Sensing System
p 2496 -176 200 0 -1 revision:
p 3008 -224 200 0 -1 title:System AG Interface Database
[comments]
