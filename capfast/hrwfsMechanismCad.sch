[schematic2]
uniq 75
[tools]
[detail]
w 1064 443 100 0 n#73 elongouts.busy.FLNK 1248 576 1312 576 1312 432 864 432 864 304 992 304 elongouts.start.SLNK
w 760 1083 100 0 n#72 ecad8.ecad8#23.VALA 288 1152 576 1152 576 1072 992 1072 estringouts.outa.DOL
w 1064 939 100 0 n#71 estringouts.outa.FLNK 1248 1056 1312 1056 1312 928 864 928 864 784 992 784 estringouts.null.SLNK
w 760 1051 100 0 n#70 ecad8.ecad8#23.STLK 288 480 576 480 576 1040 992 1040 estringouts.outa.SLNK
w 1064 674 100 0 n#64 estringouts.null.FLNK 1248 800 1312 800 1312 672 864 672 864 544 992 544 elongouts.busy.SLNK
w 1496 338 100 0 n#63 elongouts.start.FLNK 1248 336 1792 336 1792 544 1888 544 hrwfsSubCadWait.hrwfsSubCadWait#74.START
w 920 578 100 0 n#59 hwin.hwin#58.in 896 576 992 576 elongouts.busy.DOL
w 920 338 100 0 n#54 hwin.hwin#52.in 896 336 992 336 elongouts.start.DOL
w 1272 1346 100 0 n#46 ecad8.ecad8#23.VAL 288 1344 2304 1344 outhier.VAL.p
w 1542 1252 100 0 n#9 ecad8.ecad8#23.MESS 288 1312 816 1312 816 1248 2304 1248 outhier.MESS.p
w -210 1316 100 0 n#8 inhier.ICID.P -432 1248 -352 1248 -352 1312 -32 1312 ecad8.ecad8#23.ICID
w -250 1346 100 0 n#7 inhier.DIR.P -432 1344 -32 1344 ecad8.ecad8#23.DIR
s -224 -176 100 0 no parameters (so that subsystem command is marked)
s -224 -144 100 0 It is also used by the TCS for subsystem commands that have
s -224 -112 100 0 This has been done because many commands have only one parameter
s -224 -80 100 0 Note that an exception is made for the VALA field.
s 368 1856 200 0 Trigger a subsystem CAD
s 1808 2000 100 0 $Id: hrwfsMechanismCad.sch,v 1.1 2001-06-24 18:59:24 gemvx Exp $
s 1808 -336 100 0 20 October 1996
s 2032 -192 100 0 Gemini Telescope Control System
s 1760 240 100 0 This symbol handles timeouts on command acknowledgement as
s 1760 208 100 0 well as command rejection.
s -224 224 100 0 Note that the output links will send data to the input
s -224 192 100 0 fields of the subsystem CAD even if the command is
s -224 160 100 0 rejected by the TCS. The data sent will be the same
s -224 128 100 0 as in the last valid command. This is fairly
s -224 96 100 0 innocuous but will leave the subsystem CAD marked. If
s -224 64 100 0 this might cause a problem then a custom schematic should
s -224 32 100 0 be cloned from this one where the outputs are only sent 
s -224 0 100 0 when the STLK link is triggered.
[cell use]
use hrwfsSubCadWait 1888 311 100 0 hrwfsSubCadWait#74
xform 0 2024 456
p 1952 304 100 0 1 seta:top $(hrwfs)$(mech)$(cad):
p 1952 272 100 0 1 setb:index 0
use estringouts 992 967 100 0 outa
xform 0 1120 1040
p 1008 1120 100 0 -1 DESC:Output VALA from CAD
p 928 846 100 0 0 OMSL:closed_loop
p 928 1006 100 0 0 VAL:
p 1344 1024 100 0 -1 def(OUT):$(outa)
p 1104 960 100 1024 0 name:$(hrwfs)$(mech)$(cad)$(I)
use estringouts 1016 712 100 0 null
xform 0 1120 784
p 1024 878 100 0 -1 DESC:Supply an empty string
p 928 750 100 0 0 VAL:
p 1392 766 100 0 -1 def(OUT):$(hrwfs)$(prefix)CommSentC.IMSS
p 1104 704 100 1024 0 name:$(hrwfs)$(mech)$(cad)$(I)
use hwin 728 536 100 0 hwin#58
xform 0 800 576
p 707 568 100 0 -1 val(in):$(CAR_BUSY)
use hwin 728 296 100 0 hwin#52
xform 0 800 336
p 707 328 100 0 -1 val(in):$(CAD_START)
use elongouts 1016 456 100 0 busy
xform 0 1120 544
p 1040 638 100 0 -1 DESC:Busy state
p 832 462 100 0 0 OMSL:closed_loop
p 1328 510 100 0 -1 def(OUT):$(hrwfs)$(prefix)CommSentC.IVAL
p 1216 448 100 1024 0 name:$(hrwfs)$(mech)$(cad)$(I)
p 1248 512 75 768 -1 pproc(OUT):PP
use elongouts 1016 216 100 0 start
xform 0 1120 304
p 1040 398 100 0 -1 DESC:Start Directive
p 832 222 100 0 0 OMSL:closed_loop
p 1328 270 100 0 -1 def(OUT):$(subsys)$(subcad).DIR
p 1232 208 100 1024 1 name:$(hrwfs)$(mech)$(cad)$(I)
p 1248 272 75 768 -1 pproc(OUT):PP
use outhier 2296 1208 100 0 MESS
xform 0 2288 1248
use outhier 2296 1304 100 0 VAL
xform 0 2288 1344
use ecad8 -8 392 100 0 ecad8#23
xform 0 128 896
p -80 1438 100 0 -1 DESC:Triggers a subsystem CAD on START only
p 80 1142 100 0 0 FTVA:STRING
p 80 1086 100 0 0 FTVB:STRING
p 160 686 100 0 0 FTVH:STRING
p 64 736 100 0 0 PREC:4
p 48 574 100 0 -1 SNAM:$(snam)
p 352 1118 100 0 0 def(OUTA):0.0
p 352 1054 100 0 -1 def(OUTB):$(outb)
p 352 990 100 0 -1 def(OUTC):$(outc)
p 352 926 100 0 -1 def(OUTD):$(outd)
p 352 862 100 0 -1 def(OUTE):$(oute)
p 352 798 100 0 -1 def(OUTF):$(outf)
p 352 734 100 0 0 def(OUTG):0.0
p 368 670 100 0 0 def(OUTH):0.0
p 32 334 100 0 1 name:$(hrwfs)$(mech)$(cad)
p 288 1120 75 768 -1 pproc(OUTA):NPP
p 288 1056 75 768 -1 pproc(OUTB):NPP
p 288 672 75 768 -1 pproc(OUTH):NPP
use bc200tr -784 -472 -100 0 frame
xform 0 896 832
use inhier -424 1208 100 0 ICID
xform 0 -432 1248
use inhier -424 1304 100 0 DIR
xform 0 -432 1344
[comments]
