[schematic2]
uniq 353
[tools]
[detail]
w -1534 67 100 0 n#350 link2DirWfs.link2DirWfs#352.FLNK -1568 64 -1440 64 -1440 0 -1376 0 elongins.Long.SLNK
w -1342 -157 100 0 n#349 link2DirWfs.link2DirWfs#352.OUT -1568 32 -1504 32 -1504 -160 -1120 -160 -1120 -16 junction
w -1102 -5 100 0 n#349 elongins.Long.VAL -1120 -16 -1024 -16 edfans.Fan.DOL
w -670 -101 100 0 n#348 edfans.Fan.OUTE -768 -112 -512 -112 -512 -304 -288 -304 outhier.OUTE.p
w -638 -69 100 0 n#347 edfans.Fan.OUTD -768 -80 -448 -80 -448 -176 -288 -176 outhier.OUTD.p
w -670 27 100 0 n#346 edfans.Fan.OUTA -768 16 -512 16 -512 208 -288 208 outhier.OUTA.p
w -638 -5 100 0 n#345 edfans.Fan.OUTB -768 -16 -448 -16 -448 80 -288 80 outhier.OUTB.p
w -558 -37 100 0 n#344 edfans.Fan.OUTC -768 -48 -288 -48 outhier.OUTC.p
w -1084 -21 100 0 n#332 elongins.Long.FLNK -1120 16 -1088 16 -1088 -48 -1024 -48 edfans.Fan.SLNK
w -1976 -253 100 0 SPLK inhier.SPLK.P -2080 -256 -1824 -256 -1824 -64 -1760 -64 link2DirWfs.link2DirWfs#352.SPLK
w -1992 -125 100 0 STLK inhier.STLK.P -2080 -128 -1856 -128 -1856 -32 -1760 -32 link2DirWfs.link2DirWfs#352.STLK
w -1944 3 100 0 PNLK inhier.PNLK.P -2080 0 -1760 0 link2DirWfs.link2DirWfs#352.PLNK
w -1992 131 100 0 CLNK inhier.CLNK.P -2080 128 -1856 128 -1856 32 -1760 32 link2DirWfs.link2DirWfs#352.CLNK
w -1976 259 100 0 MLNK inhier.MLNK.P -2080 256 -1824 256 -1824 64 -1760 64 link2DirWfs.link2DirWfs#352.MLNK
s -2048 -240 100 0 Stop link
s -2048 -112 100 0 Start link
s -2048 16 100 0 Preset link
s -2048 144 100 0 Clear link
s -2048 272 100 0 Mark link
s -80 -1232 500 512 cadSequenceWfs
s -1072 288 100 0 This "data fanout" record triggers the CAD records of all
s -1072 240 100 0 the subsystems connected to the output simultaneously.
[cell use]
use link2DirWfs -1760 -217 100 0 link2DirWfs#352
xform 0 -1664 -48
p -1740 -244 100 0 1 set1:axis $(cad)
use notes 16 -937 100 0 notes#351
xform 0 272 -752
p 544 -786 100 0 0 AUTHOR:S.M.Beard
p 44 -626 100 0 -1 COMMENT1:This is a variation on the CICS schematic
p 44 -658 100 0 -1 COMMENT2:"cadFanout" for fanning out a directive to several
p 44 -688 100 0 -1 COMMENT3:CAD records. The main difference is that
p 44 -720 100 0 -1 COMMENT4:in the WFS system the CLEAR directive causes
p 44 -752 100 0 -1 COMMENT5:default values to be loaded, and the target
p 44 -784 100 0 -1 COMMENT6:CAD records should be marked after a CLEAR
p 44 -816 100 0 -1 COMMENT7:to have the desired effect.
p 44 -848 100 0 -1 COMMENT8:NOTE: Use with care. Using CLEAR in this way
p 44 -880 100 0 -1 COMMENT9:is not always desirable.
use outhier -320 -345 100 0 OUTE
xform 0 -304 -304
use outhier -320 -217 100 0 OUTD
xform 0 -304 -176
use outhier -320 -89 100 0 OUTC
xform 0 -304 -48
use outhier -320 39 100 0 OUTB
xform 0 -304 80
use outhier -320 167 100 0 OUTA
xform 0 -304 208
use inhier -2096 -297 100 0 SPLK
xform 0 -2080 -256
use inhier -2096 -169 100 0 STLK
xform 0 -2080 -128
use inhier -2096 -41 100 0 PNLK
xform 0 -2080 0
use inhier -2096 87 100 0 CLNK
xform 0 -2080 128
use inhier -2096 215 100 0 MLNK
xform 0 -2080 256
use elongins -1376 -73 100 0 Long
xform 0 -1248 0
p -1312 -96 100 0 1 EGU:CAD directive
p -1312 -128 100 0 1 PV:$(top)$(cad)
use edfans -1024 -265 100 0 Fan
xform 0 -896 -48
p -960 -272 100 768 1 EGU:CAD directive
p -960 -304 100 768 1 OMSL:closed_loop
p -960 -336 100 0 1 PV:$(top)$(cad)
p -768 16 75 768 -1 pproc(OUTA):PP
p -768 -16 75 768 -1 pproc(OUTB):PP
p -768 -48 75 768 -1 pproc(OUTC):PP
p -768 -80 75 768 -1 pproc(OUTD):PP
p -768 -112 75 768 -1 pproc(OUTE):PP
p -768 -144 75 768 -1 pproc(OUTF):PP
p -768 -176 75 768 -1 pproc(OUTG):PP
p -768 -208 75 768 -1 pproc(OUTH):PP
use bc200tr -2560 -1400 -100 0 frame
xform 0 -880 -96
p 0 -1232 100 0 1 author:S.M.Beard
p 240 -1248 100 0 -1 border:C
p 0 -1264 100 0 1 checked:A.Foster
p 272 -1248 100 0 -1 date:15 Oct 97
p 524 -1244 100 1792 -1 page:1
p 240 -1120 100 0 -1 project:Gemini A&G Wavefront Sensing System
p 240 -1184 100 0 -1 title:Sequence control of WFS CAD record
[comments]
