[schematic2]
uniq 27
[tools]
[detail]
s 1488 80 500 512 hrwfsSad.sch
s -368 2160 500 0 Gemini High Resolution Wavefront Sensor
[cell use]
use wfsSad 272 1383 100 0 wfsSad#26
xform 0 368 1504
p 272 1376 100 0 1 set1:wfs dc:
p 272 1344 100 0 1 set2:hindex $(sadtop)combHlt.C
p 272 1312 100 0 1 set3:mindex $(sadtop)combHlt.D
use systemSad 272 1783 100 0 systemSad#19
xform 0 368 1904
use notes 1536 279 100 0 notes#13
xform 0 1792 464
p 2064 430 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 1564 590 100 0 -1 COMMENT1:This is the under top level schematic for
p 1564 558 100 0 -1 COMMENT2:the Gemini High Resolution Wavefront Sensor
p 1564 528 100 0 -1 COMMENT3:status alarm database.
use bc200tr -1024 -104 -100 0 frame
xform 0 656 1200
p 1552 64 100 0 1 author:S.M.Beard
p 1776 48 100 0 -1 border:C
p 1552 32 100 0 1 checked:B.Goodrich
p 1776 16 100 0 -1 date:$Date: 1999-03-17 03:14:12 $
p 1552 2368 100 0 -1 id:$Id: hrwfsSad.sch,v 1.1.1.1 1999-03-17 03:14:12 cboyer Exp $
p 1792 176 100 0 -1 project:Gemini Wavefront Sensing System
p 1552 144 100 0 -1 revision:$Revision: 1.1.1.1 $
p 1792 112 100 0 -1 title:Under Top Level HRWFS Status Alarm Database
[comments]
