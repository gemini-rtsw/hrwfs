[schematic2]
uniq 16
[tools]
[detail]
s 1488 80 500 512 hrwfsTop.sch
s -368 2160 500 0 Gemini A&G Wavefront Sensing System
[cell use]
use hrwfs 32 935 100 0 hrwfs#15
xform 0 480 1232
p 256 912 100 0 1 set1:top hrwfs:
p 256 880 100 0 1 set2:sadtop hrwfs:
p 256 848 100 0 1 set3:agtop hrwfs:
p 608 912 100 0 1 seta:CAR_IDLE 0
p 608 880 100 0 1 setb:CAR_BUSY 2
use notes 1536 279 100 0 notes#13
xform 0 1792 464
p 2064 430 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 1564 590 100 0 -1 COMMENT1:This is the top level schematic for the
p 1564 558 100 0 -1 COMMENT2:Gemini A&G Wavefront Processing System
p 1564 528 100 0 -1 COMMENT3:High Resolution Wavefront Sensor and
p 1564 496 100 0 -1 COMMENT4:Acquisition Camera database.
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 1 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 2001-06-11 07:39:08 $
p 1552 2368 100 0 -1 id:$Id: hrwfsTop.sch,v 1.3 2001-06-11 07:39:08 cjm Exp $
p 1792 176 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 128 100 0 -1 revision:$Revision: 1.3 $
p 1792 112 100 0 -1 title:Top Level Schematic for HRWFS/AC Database
[comments]
