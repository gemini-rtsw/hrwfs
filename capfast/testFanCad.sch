[schematic2]
uniq 189
[tools]
[detail]
w 146 1643 100 0 n#188 efanouts.testFan.LNK2 -208 1968 32 1968 32 1632 320 1632 elongouts.testLongout3.SLNK
w 324 1659 100 2 n#187 hwin.hwin#186.in 320 1664 320 1664 elongouts.testLongout3.DOL
w 610 1675 100 0 n#185 elongouts.testLongout3.FLNK 576 1664 704 1664 704 1648 832 1648 estringouts.testString2.SLNK
w 1348 1691 100 2 n#180 hwin.hwin#163.in 1344 1696 1344 1696 elongouts.testLongout4.DOL
w 1186 1675 100 0 n#177 estringouts.testString2.FLNK 1088 1664 1344 1664 elongouts.testLongout4.SLNK
w 1266 2011 100 0 n#173 estringouts.testString1.FLNK 1088 2016 1184 2016 1184 2000 1408 2000 elongouts.testLongout2.SLNK
w 722 2011 100 0 n#172 elongouts.testLongout1.FLNK 576 2032 672 2032 672 2000 832 2000 estringouts.testString1.SLNK
w 1412 2027 100 2 n#171 hwin.hwin#140.in 1408 2032 1408 2032 elongouts.testLongout2.DOL
w 26 2011 100 0 n#168 efanouts.testFan.LNK1 -208 2000 320 2000 elongouts.testLongout1.SLNK
w -670 1931 100 0 STLK inhier.STLK.P -832 1920 -448 1920 efanouts.testFan.SLNK
w 324 2027 100 0 n#147 hwin.hwin#146.in 320 2032 320 2032 elongouts.testLongout1.DOL
s 1152 1744 100 0 START directive
s 128 2080 100 0 CLEAR directive
s 2240 -336 500 512 testFanCad.sch
s -768 2560 500 0 Wavefromt Sensing System - Test sequence CAD Records
s 1216 2080 100 0 START directive
s 128 1696 100 0 MARK directive
[cell use]
use hwin 1152 1655 100 0 hwin#163
xform 0 1248 1696
p 1155 1688 100 0 -1 val(in):3
use hwin 128 1991 100 0 hwin#146
xform 0 224 2032
p 131 2024 100 0 -1 val(in):1
use hwin 1216 1991 100 0 hwin#140
xform 0 1312 2032
p 1219 2024 100 0 -1 val(in):3
use hwin 128 1623 100 0 hwin#186
xform 0 224 1664
p 131 1656 100 0 -1 val(in):0
use elongouts 1344 1575 100 0 testLongout4
xform 0 1472 1664
p 1184 1582 100 0 0 OMSL:supervisory
p 1408 1536 100 0 1 PV:$(top)
p 1408 1504 100 0 1 def(OUT):$(cccad).DIR
p 1600 1632 75 768 -1 pproc(OUT):PP
use elongouts 320 1911 100 0 testLongout1
xform 0 448 2000
p 400 1872 100 0 1 PV:$(top)
p 400 1840 100 0 1 def(OUT):$(dccad).DIR
p 576 1968 75 768 -1 pproc(OUT):PP
use elongouts 1408 1911 100 0 testLongout2
xform 0 1536 2000
p 1248 1918 100 0 0 OMSL:supervisory
p 1472 1872 100 0 1 PV:$(top)
p 1472 1840 100 0 1 def(OUT):$(dccad).DIR
p 1664 1968 75 768 -1 pproc(OUT):PP
use elongouts 320 1543 100 0 testLongout3
xform 0 448 1632
p 160 1550 100 0 0 OMSL:supervisory
p 384 1504 100 0 1 PV:$(top)
p 384 1472 100 0 1 def(OUT):$(cccad).DIR
p 576 1600 75 768 -1 pproc(OUT):PP
use inhier -816 1920 100 1536 STLK
xform 0 -832 1920
p -880 1856 100 0 0 IO:input
p -944 1824 100 0 0 model:connector
p -944 1792 100 0 0 revision:2.2
use estringouts 832 1575 100 0 testString2
xform 0 960 1648
p 896 1536 100 0 1 PV:$(top)
p 896 1504 100 0 1 VAL:MARK
p 896 1472 100 0 1 def(OUT):$(cccad).T
use estringouts 832 1927 100 0 testString1
xform 0 960 2000
p 896 1888 100 0 1 PV:$(top)
p 896 1856 100 0 1 VAL:MARK
p 896 1824 100 0 1 def(OUT):$(dccad).T
use efanouts -448 1783 100 0 testFan
xform 0 -328 1936
p -384 1760 100 0 1 PV:$(top)
use bd200tr -1136 -584 -100 0 frame
xform 0 1504 1120
p 2496 -352 200 0 1 author:C. Boyer
p 3008 -384 100 0 0 border:D
p 2496 -432 200 0 1 checked:A.Foster
p 2992 -448 200 0 -1 date:2000/01/28
p 2480 2656 200 0 -1 id:
p 3008 -96 200 0 -1 project:Gemini Wavefront Sensing System
p 2496 -176 200 0 -1 revision:
p 3008 -224 200 0 -1 title:Test Sequence CAD Records
[comments]
