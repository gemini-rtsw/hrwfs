[schematic2]
uniq 61
[tools]
[detail]
w 700 811 100 0 n#59 wfsCar.wfsCar#56.FLNK 640 224 704 224 704 1408 junction
w 708 1659 100 0 n#59 hrwfsCar.hrwfsCar#18.FLNK 640 1920 704 1920 704 1408 1056 1408 egenSub.combActive.SLNK
w 988 1099 100 0 n#58 wfsCar.wfsCar#56.CLID 640 320 992 320 992 1888 1056 1888 egenSub.combActive.INPD
w 956 1163 100 0 n#57 wfsCar.wfsCar#56.VAL 640 384 960 384 960 1952 1056 1952 egenSub.combActive.INPC
w 1572 1659 100 0 n#48 egenSub.combActive.FLNK 1344 1376 1568 1376 1568 1952 1696 1952 carID.carID#23.SLNK
w 1576 2051 100 0 n#47 egenSub.combActive.OUTB 1344 2016 1504 2016 1504 2048 1696 2048 carID.carID#23.ICID
w 1496 2091 100 0 n#46 egenSub.combActive.OUTA 1344 2080 1696 2080 carID.carID#23.IVAL
w 824 2083 100 0 n#37 hrwfsCar.hrwfsCar#18.VAL 640 2080 1056 2080 egenSub.combActive.INPA
w 824 2019 100 0 n#36 hrwfsCar.hrwfsCar#18.CLID 640 2016 1056 2016 egenSub.combActive.INPB
s 1488 80 500 512 hrwfs.sch
s -576 2224 500 0 Gemini High Resolution Wavefront Sensor
[cell use]
use hrwfsSystemCad -608 1895 100 0 hrwfsSystemCad#60
xform 0 -512 2016
use wfsCar 448 151 100 0 wfsCar#56
xform 0 544 272
p 448 96 100 0 1 set1:wfs dc:
use wfsCad -576 199 100 0 wfsCad#55
xform 0 -480 320
p -576 192 100 0 1 set1:wfs dc:
use systemTcs -96 1895 100 0 systemTcs#53
xform 0 0 2016
use egenSub 1056 1319 100 0 combActive
xform 0 1200 1744
p 1120 1280 100 0 1 DESC:Combine CAR values
p 928 2112 100 0 1 FTA:LONG
p 928 2048 100 0 1 FTB:LONG
p 928 1984 100 0 1 FTC:LONG
p 928 1920 100 0 1 FTD:LONG
p 928 1856 100 0 1 FTE:LONG
p 928 1792 100 0 1 FTF:LONG
p 928 1728 100 0 1 FTG:LONG
p 928 1664 100 0 1 FTH:LONG
p 928 1600 100 0 1 FTI:LONG
p 928 1536 100 0 1 FTJ:LONG
p 1376 2112 100 0 1 FTVA:LONG
p 1376 2048 100 0 1 FTVB:LONG
p 1376 1984 100 0 1 FTVC:LONG
p 1120 1216 100 0 1 PV:$(top)
p 1120 1248 100 0 1 SNAM:cicsCarValCombine
p 833 1093 100 0 0 UFC:
use carID 1696 1831 100 0 carID#23
xform 0 1792 1984
p 1696 1824 100 0 1 set1:car Top level
p 1696 1792 100 0 1 set2:pv $(top)active
use hrwfsCar 448 1847 100 0 hrwfsCar#18
xform 0 544 1968
use notes 1536 279 100 0 notes#13
xform 0 1792 464
p 2064 430 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 1564 590 100 0 -1 COMMENT1:This is the under top level schematic for
p 1564 558 100 0 -1 COMMENT2:the Gemini High Resolution Wavefront Sensor
p 1564 528 100 0 -1 COMMENT3:main database.
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 1 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 1999-03-17 03:14:11 $
p 1552 2368 100 0 -1 id:$Id: hrwfs.sch,v 1.1.1.1 1999-03-17 03:14:11 cboyer Exp $
p 1792 176 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 144 100 0 -1 revision:$Revision: 1.1.1.1 $
p 1792 112 100 0 -1 title:Under Top Level Schematic for HRWFS Database
[comments]
