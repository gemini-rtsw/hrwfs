[schematic2]
uniq 189
[tools]
[detail]
w 594 2011 100 0 n#188 elongouts.parkLongout1.FLNK 448 2048 544 2048 544 2000 704 2000 estringouts.parkString.SLNK
w -134 2011 100 0 n#187 efanouts.parkFan.LNK1 -208 2000 0 2000 0 2016 192 2016 elongouts.parkLongout1.SLNK
w 196 2043 100 2 n#186 hwin.hwin#185.in 192 2048 192 2048 elongouts.parkLongout1.DOL
w 1220 2043 100 2 n#180 hwin.hwin#163.in 1216 2048 1216 2048 elongouts.parkLongout2.DOL
w 1058 2027 100 0 n#177 estringouts.parkString.FLNK 960 2016 1216 2016 elongouts.parkLongout2.SLNK
w -670 1931 100 0 STLK inhier.STLK.P -832 1920 -448 1920 efanouts.parkFan.SLNK
s 0 2080 100 0 NARK directive
s 1024 2096 100 0 START directive
s 2240 -336 500 512 parkFanCad.sch
s -768 2560 500 0 Wavefromt Sensing System - Park sequence CAD Records
[cell use]
use hwin 0 2007 100 0 hwin#185
xform 0 96 2048
p 3 2040 100 0 -1 val(in):0
use hwin 1024 2007 100 0 hwin#163
xform 0 1120 2048
p 1027 2040 100 0 -1 val(in):3
use elongouts 192 1927 100 0 parkLongout1
xform 0 320 2016
p 32 1934 100 0 0 OMSL:supervisory
p 256 1888 100 0 1 PV:$(top)
p 256 1856 100 0 1 def(OUT):$(cccad).DIR
p 448 1984 75 768 -1 pproc(OUT):PP
use elongouts 1216 1927 100 0 parkLongout2
xform 0 1344 2016
p 1056 1934 100 0 0 OMSL:supervisory
p 1280 1888 100 0 1 PV:$(top)
p 1280 1856 100 0 1 def(OUT):$(cccad).DIR
p 1472 1984 75 768 -1 pproc(OUT):PP
use inhier -816 1920 100 1536 STLK
xform 0 -832 1920
p -880 1856 100 0 0 IO:input
p -944 1824 100 0 0 model:connector
p -944 1792 100 0 0 revision:2.2
use estringouts 704 1927 100 0 parkString
xform 0 832 2000
p 768 1888 100 0 1 PV:$(top)
p 768 1856 100 0 1 VAL:MARK
p 768 1824 100 0 1 def(OUT):$(cccad).T
use efanouts -448 1783 100 0 parkFan
xform 0 -328 1936
p -384 1760 100 0 1 PV:$(top)
use bd200tr -1136 -584 -100 0 frame
xform 0 1504 1120
p 2496 -352 200 0 1 author:C. Boyer
p 3008 -384 100 0 0 border:D
p 2496 -432 200 0 1 checked:
p 2992 -448 200 0 -1 date:2000/01/27
p 2480 2656 200 0 -1 id:
p 3008 -96 200 0 -1 project:Gemini Wavefront Sensing System
p 2496 -176 200 0 -1 revision:
p 3008 -224 200 0 -1 title:Park Sequence CAD Records
[comments]
