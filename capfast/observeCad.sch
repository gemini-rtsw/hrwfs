[schematic2]
uniq 79
[tools]
[detail]
s -544 2528 500 0 Wavefront Sensing - Observation CAD Records
s 2240 -336 500 512 observeCad.sch
[cell use]
use ecad8 1248 775 100 0 calibrate
xform 0 1408 1280
p 1312 736 100 0 1 DESC:Calibrate one or more WFSs
p 1360 1536 100 0 1 FTVA:LONG
p 1360 1472 100 0 1 FTVB:LONG
p 1360 1408 100 0 1 FTVC:LONG
p 1360 1344 100 0 1 FTVD:LONG
p 1360 1280 100 0 1 FTVE:LONG
p 1360 1216 100 0 1 FTVF:STRING
p 1360 1152 100 0 1 FTVG:STRING
p 1360 1088 100 0 1 FTVH:STRING
p 1312 704 100 0 1 INAM:epToVxCadInit
p 1312 640 100 0 1 PV:$(top)
p 1312 672 100 0 1 SNAM:epToVxCadExecute
use ecad8 672 775 100 0 gbdObserve
xform 0 832 1280
p 736 736 100 0 1 DESC:Make one-off or continuous observation
p 784 1536 100 0 1 FTVA:LONG
p 784 1472 100 0 1 FTVB:LONG
p 784 1408 100 0 1 FTVC:LONG
p 784 1344 100 0 1 FTVD:LONG
p 784 1280 100 0 1 FTVE:LONG
p 784 1216 100 0 1 FTVF:STRING
p 784 1152 100 0 1 FTVG:STRING
p 784 1088 100 0 1 FTVH:STRING
p 736 704 100 0 1 INAM:epToVxCadInit
p 736 640 100 0 1 PV:$(top)
p 736 672 100 0 1 SNAM:epToVxCadExecute
use bd200tr -1136 -584 -100 0 frame
xform 0 1504 1120
p 2496 -352 200 0 1 author:S.M.Beard
p 3008 -384 100 0 0 border:D
p 2496 -432 200 0 1 checked:A.Foster
p 2992 -448 200 0 -1 date:$Date: 1999-03-17 03:14:14 $
p 2480 2656 200 0 -1 id:$Id: observeCad.sch,v 1.1.1.1 1999-03-17 03:14:14 cboyer Exp $
p 3008 -96 200 0 -1 project:Gemini Wavefront Sensing System
p 2496 -176 200 0 -1 revision:$Revision: 1.1.1.1 $
p 3008 -224 200 0 -1 title:One-off observation CAD Records
use notes 3456 55 100 0 notes#13
xform 0 3712 240
p 3984 206 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3484 366 100 0 -1 COMMENT1:This schematic contains the CAD records
p 3484 334 100 0 -1 COMMENT2:for the subset of the systemwide AGWPS
p 3484 304 100 0 -1 COMMENT3:devoted to making one-off observations.
p 3484 272 100 0 -1 COMMENT4:.
p 3484 240 100 0 -1 COMMENT5:See ICD 1.6.2/1.6.3 for a description
p 3484 208 100 0 -1 COMMENT6:of what these commands do.
[comments]
