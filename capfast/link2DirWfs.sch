[schematic2]
uniq 197
[tools]
[detail]
w 738 2059 100 0 n#190 eseqs.put1.LNK2 720 2048 816 2048 816 2080 junction
w 834 1771 100 0 n#194 eseqs.put1.FLNK 720 1760 1008 1760 junction
w 300 1995 100 0 n#187 hwin.hwin#188.in 272 1952 304 1952 304 2048 400 2048 eseqs.put1.DOL2
w 1004 539 100 0 n#194 elongouts.put4.FLNK 720 256 1008 256 1008 832 junction
w 1004 971 100 0 n#194 eseqs.put3.LNK1 736 832 1008 832 1008 1120 junction
w 1004 1627 100 0 n#194 eseqs.put2.FLNK 720 1120 1008 1120 1008 2144 junction
w 834 2371 100 0 n#194 elongouts.put0.FLNK 720 2368 1008 2368 1008 2144 junction
w 1202 2147 100 0 n#194 junction 1008 2144 1456 2144 outhier.FLNK.p
w 876 2187 100 0 n#190 elongouts.put0.OUT 720 2304 880 2304 880 2080 junction
w 876 1755 100 0 n#190 eseqs.put1.LNK1 720 2080 880 2080 880 1440 junction
w 876 971 100 0 n#190 eseqs.put2.LNK1 720 1440 880 1440 880 512 junction
w 876 347 100 0 n#190 elongouts.put4.OUT 720 192 880 192 880 512 junction
w 1066 515 100 0 n#190 eseqs.put3.FLNK 736 512 1456 512 1456 592 outhier.OUT.p
w 66 235 100 0 n#165 inhier.SPLK.P -272 224 464 224 elongouts.put4.SLNK
w 42 515 100 0 n#164 inhier.STLK.P -272 512 416 512 eseqs.put3.SLNK
w 42 1123 100 0 n#162 inhier.PLNK.P -256 1120 400 1120 eseqs.put2.SLNK
w 34 1763 100 0 n#161 inhier.CLNK.P -272 1760 400 1760 eseqs.put1.SLNK
w 66 2347 100 0 n#160 inhier.MLNK.P -272 2336 464 2336 elongouts.put0.SLNK
w 424 2379 100 0 n#57 hwin.hwin#12.in 432 2368 464 2368 elongouts.put0.DOL
w 312 2083 100 0 n#55 hwin.hwin#8.in 272 2080 400 2080 eseqs.put1.DOL1
w 312 1443 100 0 n#53 hwin.hwin#7.in 272 1440 400 1440 eseqs.put2.DOL1
w 312 835 100 0 n#49 hwin.hwin#9.in 256 832 416 832 eseqs.put3.DOL1
w 424 267 100 0 n#41 hwin.hwin#10.in 432 256 464 256 elongouts.put4.DOL
s 80 1984 100 0 CAD_MARK = 0
s 240 288 100 0 CAD_STOP = 4
s 64 880 100 0 CAD_START = 3
s 80 1488 100 0 CAD_PRESET = 2
s 80 2112 100 0 CAD_CLEAR = 1
s 240 2416 100 0 CAD_MARK = 0
s 1984 128 500 512 link2DirWfs.sch
s -224 2368 100 0 Mark link
s -224 1776 100 0 Clear link
s -208 1136 100 0 Preset link
s -224 528 100 0 Start link
s -224 256 100 0 Stop link
s 1408 608 100 512 CAD directive outlink
s 1408 2160 100 512 Forward link
[cell use]
use eseqs 416 423 100 0 put3
xform 0 576 672
p 480 352 100 0 1 DLY1:0.2e+00
p 480 320 100 0 1 PV:$(top)$(mech)$(axis):
p 480 384 100 0 1 SELM:All
p 752 832 75 1024 -1 pproc(LNK1):PP
use eseqs 400 1671 100 0 put1
xform 0 560 1920
p 464 1600 100 0 1 DLY1:0.0e+00
p 464 1568 100 0 1 DLY2:0.1e+00
p 464 1536 100 0 1 PV:$(top)$(mech)$(axis):
p 464 1632 100 0 1 SELM:All
p 736 2080 75 1024 -1 pproc(LNK1):PP
p 736 2048 75 1024 -1 pproc(LNK2):PP
use eseqs 400 1031 100 0 put2
xform 0 560 1280
p 464 960 100 0 1 DLY1:0.2e+00
p 464 928 100 0 1 PV:$(top)$(mech)$(axis):
p 464 992 100 0 1 SELM:All
p 736 1440 75 1024 -1 pproc(LNK1):PP
use hwin 80 1911 100 0 hwin#188
xform 0 176 1952
p 83 1944 100 0 -1 val(in):0
use inhier -288 183 100 0 SPLK
xform 0 -272 224
use inhier -288 471 100 0 STLK
xform 0 -272 512
use inhier -288 1719 100 0 CLNK
xform 0 -272 1760
use inhier -272 1079 100 0 PLNK
xform 0 -256 1120
use inhier -288 2295 100 0 MLNK
xform 0 -272 2336
use bc200tr -512 -24 -100 0 frame
xform 0 1168 1280
p 2064 144 100 0 1 author:S.M.Beard
p 2288 128 100 0 -1 border:C
p 2048 112 100 0 1 checked:A.Foster
p 2320 128 100 0 -1 date:15 Oct 98
p -512 -24 100 0 0 id:frame
p 2288 256 100 0 -1 project:Gemini A&G Wavefront Processing System
p 2288 192 100 0 -1 title:Convert link to CAD directive - WFS variant
use elongouts 464 2247 100 0 put0
xform 0 592 2336
p 528 2240 100 0 1 EGU:CAD directive
p 528 2208 100 0 1 OMSL:closed_loop
p 528 2176 100 0 1 PV:$(top)$(mech)$(axis):
p 720 2304 75 768 -1 pproc(OUT):PP
use elongouts 464 135 100 0 put4
xform 0 592 224
p 528 128 100 0 1 EGU:CAD directive
p 528 96 100 0 1 OMSL:closed_loop
p 528 64 100 0 1 PV:$(top)$(mech)$(axis):
p 720 192 75 768 -1 pproc(OUT):PP
use outhier 1424 551 100 0 OUT
xform 0 1440 592
use outhier 1424 2103 100 0 FLNK
xform 0 1440 2144
use hwin 80 2039 100 0 hwin#8
xform 0 176 2080
p 83 2072 100 0 -1 val(in):1
use hwin 240 2327 100 0 hwin#12
xform 0 336 2368
p 243 2360 100 0 -1 val(in):0
use hwin 240 215 100 0 hwin#10
xform 0 336 256
p 243 248 100 0 -1 val(in):4
use hwin 64 791 100 0 hwin#9
xform 0 160 832
p 67 824 100 0 -1 val(in):3
use hwin 80 1399 100 0 hwin#7
xform 0 176 1440
p 83 1432 100 0 -1 val(in):2
[comments]
