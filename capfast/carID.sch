[schematic2]
uniq 36
[tools]
[detail]
w 888 1579 100 0 OVAL ecars.C.VAL 832 1568 992 1568 992 1728 1088 1728 outhier.VAL.p
w -24 1931 100 0 IVAL inhier.IVAL.P -448 1920 448 1920 448 1568 512 1568 ecars.C.IVAL
w -56 1163 100 0 n#32 inhier.IERR.P -448 1152 384 1152 384 1472 512 1472 ecars.C.IERR
w -72 1347 100 0 n#31 inhier.IMSS.P -448 1344 352 1344 352 1504 512 1504 ecars.C.IMSS
w 996 1307 100 0 n#28 ecars.C.OERR 832 1472 992 1472 992 1152 1088 1152 outhier.OERR.p
w 904 1515 100 0 n#27 ecars.C.OMSS 832 1504 1024 1504 1024 1344 1088 1344 outhier.OMSS.p
w -40 963 100 0 n#24 inhier.SLNK.P -448 960 416 960 416 1376 512 1376 ecars.C.SLNK
w -104 1731 100 0 n#23 inhier.ICID.P -448 1728 288 1728 288 1536 junction
w 360 1547 100 0 n#23 elongins.CID.VAL 256 1536 512 1536 ecars.C.ICID
w 936 1547 100 0 n#21 ecars.C.CLID 832 1536 1088 1536 outhier.CLID.p
w 892 1147 100 0 n#20 ecars.C.FLNK 832 1344 896 1344 896 960 1088 960 outhier.FLNK.p
s 1488 80 500 512 carID.sch
s -80 2176 500 0 CAR record plus client ID record
[cell use]
use inhier -464 1111 100 0 IERR
xform 0 -448 1152
use inhier -464 1303 100 0 IMSS
xform 0 -448 1344
use inhier -464 919 100 0 SLNK
xform 0 -448 960
use inhier -464 1687 100 0 ICID
xform 0 -448 1728
use inhier -464 1879 100 0 IVAL
xform 0 -448 1920
use outhier 1056 1111 100 0 OERR
xform 0 1072 1152
use outhier 1056 1303 100 0 OMSS
xform 0 1072 1344
use outhier 1056 919 100 0 FLNK
xform 0 1072 960
use outhier 1056 1495 100 0 CLID
xform 0 1072 1536
use outhier 1056 1687 100 0 VAL
xform 0 1072 1728
use elongins 0 1479 100 0 CID
xform 0 128 1552
p 64 1456 100 0 1 DESC:$(car) client ID
p 64 1424 100 0 1 EGU:client ID
p 64 1392 100 0 1 PV:$(pv)
use ecars 512 1287 100 0 C
xform 0 672 1456
p 576 1248 100 0 1 DESC:$(car) CAR record
p 576 1216 100 0 1 PV:$(pv)
use notes 1536 279 100 0 notes#13
xform 0 1792 464
p 2064 430 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 1564 590 100 0 -1 COMMENT1:This schematic packages together
p 1564 558 100 0 -1 COMMENT2:a common combination of CAR
p 1564 528 100 0 -1 COMMENT3:record and client ID storage record.
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 1 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 1999-03-17 03:14:11 $
p 1552 2368 100 0 -1 id:$Id: carID.sch,v 1.1.1.1 1999-03-17 03:14:11 cboyer Exp $
p 1792 176 100 0 -1 project:Gemini Wavefront Sensing System
p 1792 112 100 0 -1 title:CAR record plus client ID record
[comments]
