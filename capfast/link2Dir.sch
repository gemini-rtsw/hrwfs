[schematic2]
uniq 184
[tools]
[detail]
w 1426 1635 100 0 n#183 elongouts.put1.FLNK 1088 1632 1824 1632 outhier.FLNK.p
w 1202 1891 100 0 n#183 elongouts.put0.FLNK 1088 1888 1376 1888 1376 1632 junction
w 1202 1379 100 0 n#183 elongouts.put2.FLNK 1088 1376 1376 1376 1376 1632 junction
w 1202 1123 100 0 n#183 elongouts.put3.FLNK 1088 1120 1376 1120 1376 1376 junction
w 1202 867 100 0 n#183 elongouts.put4.FLNK 1088 864 1376 864 1376 1120 junction
w 1426 1059 100 0 n#172 elongouts.put3.OUT 1088 1056 1824 1056 outhier.OUT.p
w 1244 923 100 0 n#172 elongouts.put4.OUT 1088 800 1248 800 1248 1056 junction
w 1244 1179 100 0 n#172 elongouts.put2.OUT 1088 1312 1248 1312 1248 1056 junction
w 1244 1435 100 0 n#172 elongouts.put1.OUT 1088 1568 1248 1568 1248 1312 junction
w 1244 1691 100 0 n#172 elongouts.put0.OUT 1088 1824 1248 1824 1248 1568 junction
w 434 843 100 0 n#165 inhier.SPLK.P 96 832 832 832 elongouts.put4.SLNK
w 434 1099 100 0 n#164 inhier.STLK.P 96 1088 832 1088 elongouts.put3.SLNK
w 434 1355 100 0 n#162 inhier.PLNK.P 96 1344 832 1344 elongouts.put2.SLNK
w 434 1611 100 0 n#161 inhier.CLNK.P 96 1600 832 1600 elongouts.put1.SLNK
w 434 1867 100 0 n#160 inhier.MLNK.P 96 1856 832 1856 elongouts.put0.SLNK
w 792 1899 100 0 n#57 hwin.hwin#12.in 800 1888 832 1888 elongouts.put0.DOL
w 792 1643 100 0 n#55 hwin.hwin#8.in 800 1632 832 1632 elongouts.put1.DOL
w 792 1387 100 0 n#53 hwin.hwin#7.in 800 1376 832 1376 elongouts.put2.DOL
w 792 1131 100 0 n#49 hwin.hwin#9.in 800 1120 832 1120 elongouts.put3.DOL
w 792 875 100 0 n#41 hwin.hwin#10.in 800 864 832 864 elongouts.put4.DOL
s 1776 1648 100 512 Forward link
s 1776 1072 100 512 CAD directive outlink
s 144 864 100 0 Stop link
s 128 1104 100 0 Start link
s 144 1360 100 0 Preset link
s 144 1616 100 0 Clear link
s 144 1888 100 0 Mark link
s -144 2160 500 0 Convert Link Input to CAD Directive Output
s 1984 128 500 512 link2Dir.sch
s 608 1936 100 0 CAD_MARK = 0
s 608 1664 100 0 CAD_CLEAR = 1
s 608 1424 100 0 CAD_PRESET = 2
s 608 1168 100 0 CAD_START = 3
s 608 896 100 0 CAD_STOP = 4
n 1824 512 2304 864 100
This schematic converts an input link
into a CAD directive. For example, if
the "Start link", STLK, is activated
the value 3 (=CAD_START) will be written
to the output.
.
Note that EPICS suffers from the
deficiency that it cannot readily
use constants declared in header files,
so the values CAD_MARK, CAD_CLEAR etc...
have to be declared explicitly. Using
this schematic will at least restrict
the use of these explicit declarations
to one place.
_
[cell use]
use inhier 80 1815 100 0 MLNK
xform 0 96 1856
use inhier 80 1303 100 0 PLNK
xform 0 96 1344
use inhier 80 1559 100 0 CLNK
xform 0 96 1600
use inhier 80 1047 100 0 STLK
xform 0 96 1088
use inhier 80 791 100 0 SPLK
xform 0 96 832
use bc200tr -512 -24 -100 0 frame
xform 0 1168 1280
p 2064 144 100 0 1 author:S.M.Beard
p 2288 128 100 0 -1 border:C
p 2048 112 100 0 1 checked:B.Goodrich
p 2320 128 100 0 -1 date:21 Jan 97
p -512 -24 100 0 0 id:frame
p 2288 256 100 0 -1 project:Core Instrument Control System
p 2288 192 100 0 -1 title:Convert link to CAD directive
use elongouts 832 743 100 0 put4
xform 0 960 832
p 896 736 100 0 1 EGU:CAD directive
p 896 704 100 0 1 OMSL:closed_loop
p 896 672 100 0 1 PV:$(top)$(mech)$(axis):
p 1088 800 75 768 -1 pproc(OUT):PP
use elongouts 832 999 100 0 put3
xform 0 960 1088
p 896 992 100 0 1 EGU:CAD directive
p 896 960 100 0 1 OMSL:closed_loop
p 896 928 100 0 1 PV:$(top)$(mech)$(axis):
p 1088 1056 75 768 -1 pproc(OUT):PP
use elongouts 832 1255 100 0 put2
xform 0 960 1344
p 896 1248 100 0 1 EGU:CAD directive
p 896 1216 100 0 1 OMSL:closed_loop
p 896 1184 100 0 1 PV:$(top)$(mech)$(axis):
p 1088 1312 75 768 -1 pproc(OUT):PP
use elongouts 832 1511 100 0 put1
xform 0 960 1600
p 896 1504 100 0 1 EGU:CAD directive
p 896 1472 100 0 1 OMSL:closed_loop
p 896 1440 100 0 1 PV:$(top)$(mech)$(axis):
p 1088 1568 75 768 -1 pproc(OUT):PP
use elongouts 832 1767 100 0 put0
xform 0 960 1856
p 896 1760 100 0 1 EGU:CAD directive
p 896 1728 100 0 1 OMSL:closed_loop
p 896 1696 100 0 1 PV:$(top)$(mech)$(axis):
p 1088 1824 75 768 -1 pproc(OUT):PP
use outhier 1792 1591 100 0 FLNK
xform 0 1808 1632
use outhier 1792 1015 100 0 OUT
xform 0 1808 1056
use hwin 608 1335 100 0 hwin#7
xform 0 704 1376
p 611 1368 100 0 -1 val(in):2
use hwin 608 1079 100 0 hwin#9
xform 0 704 1120
p 611 1112 100 0 -1 val(in):3
use hwin 608 823 100 0 hwin#10
xform 0 704 864
p 611 856 100 0 -1 val(in):4
use hwin 608 1847 100 0 hwin#12
xform 0 704 1888
p 611 1880 100 0 -1 val(in):0
use hwin 608 1591 100 0 hwin#8
xform 0 704 1632
p 611 1624 100 0 -1 val(in):1
[comments]
