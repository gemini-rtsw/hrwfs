[schematic2]
uniq 78
[tools]
[detail]
s 2512 -704 500 512 resourceSad.sch
s -464 2224 500 0 Wavefront Sensing - Resource Usage Status Records
[cell use]
use esirs 608 1735 100 0 cpuUsed00
xform 0 816 1888
p 672 1696 100 0 1 DESC:CPU usage for processor 00
p 672 1632 100 0 1 EGU:%
p 544 1472 100 0 0 FDSC:CPU usage for processor 00
p 672 1664 100 0 1 FTVL:LONG
p 832 1440 100 0 1 HHSV:MAJOR
p 832 1600 100 0 1 HIGH:90
p 832 1568 100 0 1 HIHI:97
p 832 1536 100 0 1 HOPR:100
p 672 1440 100 0 1 HSV:MINOR
p 672 1568 100 0 1 LOLO:0
p 672 1536 100 0 1 LOPR:0
p 672 1600 100 0 1 LOW:0
p 672 1472 100 0 1 PV:$(sadtop)
p 672 1504 100 0 1 SNAM:
use esirs 1888 1735 100 0 ramUsed00
xform 0 2096 1888
p 1952 1696 100 0 1 DESC:Used RAM for processor 00
p 1952 1632 100 0 1 EGU:%
p 1824 1472 100 0 0 FDSC:Used RAM for processor 00
p 1952 1664 100 0 1 FTVL:LONG
p 1824 1408 100 0 0 HHSV:MAJOR
p 2112 1600 100 0 1 HIGH:75
p 2112 1568 100 0 1 HIHI:90
p 2112 1536 100 0 1 HOPR:100
p 1824 1312 100 0 0 HSV:MINOR
p 1824 1248 100 0 0 LLSV:NO_ALARM
p 1952 1568 100 0 1 LOLO:0
p 1952 1536 100 0 1 LOPR:0
p 1952 1600 100 0 1 LOW:0
p 1952 1472 100 0 1 PV:$(sadtop)
p 1952 1504 100 0 1 SNAM:
use esirs 1248 1735 100 0 ramFreeblk00
xform 0 1456 1888
p 1312 1696 100 0 1 DESC:Free RAM for processor 00
p 1312 1632 100 0 1 EGU:Kbytes
p 1184 1472 100 0 0 FDSC:Free RAM for processor 00
p 1312 1664 100 0 1 FTVL:LONG
p 1184 1376 100 0 0 HIGH:1024000.0
p 1184 1344 100 0 0 HIHI:1024000.0
p 1184 1248 100 0 0 LLSV:MAJOR
p 1184 1216 100 0 0 LOLO:16.0
p 1184 1184 100 0 0 LOW:512.0
p 1184 1152 100 0 0 LSV:MINOR
p 1312 1504 100 0 1 PV:$(sadtop)
p 1312 1536 100 0 1 SNAM:
use notes 3520 -265 100 0 notes#13
xform 0 3776 -80
p 4048 -114 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3548 46 100 0 -1 COMMENT1:This schematic contains the resource
p 3548 14 100 0 -1 COMMENT2:usage records.
p 3548 -48 100 0 -1 COMMENT4:NOTE: These resource usage records assume
p 3548 -80 100 0 -1 COMMENT5:there are between 1 and 3 processors.
p 3548 -112 100 0 -1 COMMENT6:It would be more flexible to put each
p 3548 -144 100 0 -1 COMMENT7:set of these records in a separate
p 3548 -176 100 0 -1 COMMENT8:"procSad.sch" schematic. SMB - 4/2/98.
use bd200tr -1024 -904 -100 0 frame
xform 0 1616 800
p 2608 -672 200 0 1 author:S.M.Beard
p 3120 -704 100 0 0 border:D
p 2608 -752 200 0 1 checked:B.Goodrich
p 3184 -688 200 0 -1 date:$Date: 1999-11-04 04:03:03 $
p 2592 2336 200 0 -1 id:$Id: resourceSad.sch,v 1.2 1999-11-04 04:03:03 cboyer Exp $
p 3120 -416 200 0 -1 project:Gemini Wavefront Sensing System
p 2592 -480 200 0 -1 revision:$Revision: 1.2 $
p 3120 -544 200 0 -1 title:Resource Usage Status Records
[comments]
