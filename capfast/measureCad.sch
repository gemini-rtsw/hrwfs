[schematic2]
uniq 79
[tools]
[detail]
s -880 2528 500 0 Wavefront Sensing - Wavefront Measurement CAD Records
s 2240 -336 500 512 measureCad.sch
[cell use]
use ecad8 864 711 100 0 startMeasure
xform 0 1024 1216
p 928 672 100 0 1 DESC:Start wavefront measurement
p 976 1472 100 0 1 FTVA:LONG
p 976 1408 100 0 1 FTVB:LONG
p 976 1344 100 0 1 FTVC:LONG
p 976 1280 100 0 1 FTVD:LONG
p 976 1216 100 0 1 FTVE:LONG
p 976 1152 100 0 1 FTVF:LONG
p 976 1088 100 0 1 FTVG:LONG
p 976 1024 100 0 0 FTVH:STRING
p 928 640 100 0 1 INAM:epToVxCadInit
p 928 576 100 0 1 PV:$(top)
p 928 608 100 0 1 SNAM:epToVxCadExecute
use ecad2 352 1095 100 0 setRouter
xform 0 512 1408
p 416 1056 100 0 1 DESC:Select pixel router ISS port
p 464 1472 100 0 1 FTVA:LONG
p 464 1408 100 0 1 FTVB:LONG
p 416 1024 100 0 1 INAM:epToVxCadInit
p 416 960 100 0 1 PV:$(top)
p 416 992 100 0 1 SNAM:epToVxCadExecute
use ecad2 1376 1095 100 0 stopMeasure
xform 0 1536 1408
p 1440 1056 100 0 1 DESC:Stop wavefront measurement
p 1488 1472 100 0 0 FTVA:STRING
p 1488 1408 100 0 0 FTVB:STRING
p 1440 1024 100 0 1 INAM:epToVxCadInit
p 1440 960 100 0 1 PV:$(top)
p 1440 992 100 0 1 SNAM:epToVxCadExecute
use bd200tr -1136 -584 -100 0 frame
xform 0 1504 1120
p 2496 -352 200 0 1 author:S.M.Beard
p 3008 -384 100 0 0 border:D
p 2496 -432 200 0 1 checked:A.Foster
p 2992 -448 200 0 -1 date:$Date: 1999-03-17 03:14:14 $
p 2480 2656 200 0 -1 id:$Id: measureCad.sch,v 1.1.1.1 1999-03-17 03:14:14 cboyer Exp $
p 3008 -96 200 0 -1 project:Gemini Wavefront Sensing System
p 2496 -176 200 0 -1 revision:$Revision: 1.1.1.1 $
p 3008 -224 200 0 -1 title:Wavefront Measurement CAD Records
use notes 3456 55 100 0 notes#13
xform 0 3712 240
p 3984 206 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3484 366 100 0 -1 COMMENT1:This schematic contains the CAD records
p 3484 334 100 0 -1 COMMENT2:for the subset of the systemwide AGWPS
p 3484 304 100 0 -1 COMMENT3:commands devoted to wavefront measurement.
p 3484 272 100 0 -1 COMMENT4:.
p 3484 240 100 0 -1 COMMENT5:See ICD 1.6.2/1.6.3 for a description
p 3484 208 100 0 -1 COMMENT6:of what these commands do.
[comments]
