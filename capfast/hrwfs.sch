[schematic2]
uniq 77
[tools]
[detail]
w 424 1899 100 0 n#75 hrwfsApply.hrwfsApply#72.CLID 352 1888 544 1888 hrwfsCarTree.hrwfsCarTree#73.ICID
w 424 1643 100 0 n#74 hrwfsApply.hrwfsApply#72.OLNK 352 1632 544 1632 hrwfsCarTree.hrwfsCarTree#73.IVAL
s -576 2224 500 0 Gemini High Resolution Wavefront Sensor
s 1488 80 500 512 hrwfs.sch
[cell use]
use hrwfsCarTree 544 1575 100 0 hrwfsCarTree#73
xform 0 720 1760
use hrwfsApply 32 1575 100 0 hrwfsApply#72
xform 0 176 1760
p 52 1548 100 0 1 seta:wfs dc:
use systemAg 608 1031 100 0 systemAg#71
xform 0 704 1152
use systemTcs 96 1031 100 0 systemTcs#53
xform 0 192 1152
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
p 1776 16 100 0 -1 date:$Date: 2001-06-24 18:51:04 $
p 1552 2368 100 0 -1 id:$Id: hrwfs.sch,v 1.7 2001-06-24 18:51:04 gemvx Exp $
p 1792 176 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 144 100 0 -1 revision:$Revision: 1.7 $
p 1792 112 100 0 -1 title:Under Top Level Schematic for HRWFS Database
[comments]
