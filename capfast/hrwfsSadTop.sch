[schematic2]
uniq 17
[tools]
[detail]
s -368 2160 500 0 Gemini A&G Wavefront Sensing System
s 1488 80 500 512 hrwfsSadTop.sch
[cell use]
use hrwfsSad 80 1063 100 0 hrwfsSad#16
xform 0 560 1264
p 80 1040 100 0 1 set1:top hrwfs:
p 80 1008 100 0 1 set2:sadtop hrwfs:
use notes 1536 279 100 0 notes#13
xform 0 1792 464
p 2064 430 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 1564 590 100 0 -1 COMMENT1:This is the top level schematic for the
p 1564 558 100 0 -1 COMMENT2:Gemini A&G Wavefront Processing System
p 1564 528 100 0 -1 COMMENT3:HRWFS/AC status alarm database.
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 1 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 1999-03-17 03:14:12 $
p 1552 2368 100 0 -1 id:$Id: hrwfsSadTop.sch,v 1.1.1.1 1999-03-17 03:14:12 cboyer Exp $
p 1792 176 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 144 100 0 -1 revision:$Revision: 1.1.1.1 $
p 1792 112 100 0 -1 title:Top Level HRWFS/AC Status Alarm Database
[comments]
