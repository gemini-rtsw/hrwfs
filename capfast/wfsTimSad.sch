[schematic2]
uniq 100
[tools]
[detail]
s 2464 -704 500 512 wfsTimSad.sch
s -320 2176 500 0 Wavefront Sensing - WFS Timing Status Records
[cell use]
use esirs 160 775 100 0 exposed
xform 0 368 928
p 224 736 100 0 1 DESC:Actual total integration time
p 224 640 100 0 1 EGU:seconds
p 96 512 100 0 0 FDSC:Actual total integration time
p 224 704 100 0 1 FTVL:DOUBLE
p 224 608 100 0 1 PREC:4
p 224 672 100 0 1 PV:$(sadtop)$(wfs)
use esirs 160 1255 100 0 exposedRQ
xform 0 368 1408
p 224 1216 100 0 1 DESC:Requested total integration time
p 224 1120 100 0 1 EGU:seconds
p 96 992 100 0 0 FDSC:Requested total integration time
p 224 1184 100 0 1 FTVL:DOUBLE
p 224 1088 100 0 1 PREC:4
p 224 1152 100 0 1 PV:$(sadtop)$(wfs)
use esirs 768 775 100 0 utend
xform 0 976 928
p 832 736 100 0 1 DESC:UT at end of observation
p 832 640 100 0 1 EGU:Universal Time
p 704 512 100 0 0 FDSC:UT at end of observation
p 832 704 100 0 1 FTVL:STRING
p 832 672 100 0 1 PV:$(sadtop)$(wfs)
use esirs 768 1255 100 0 utstart
xform 0 976 1408
p 832 1216 100 0 1 DESC:UT at start of observation
p 832 1120 100 0 1 EGU:Universal Time
p 704 992 100 0 0 FDSC:UT at start of observation
p 832 1184 100 0 1 FTVL:STRING
p 832 1152 100 0 1 PV:$(sadtop)$(wfs)
use esirs 1376 1239 100 0 elapsed
xform 0 1584 1392
p 1440 1200 100 0 1 DESC:Total elapsed time
p 1440 1104 100 0 1 EGU:seconds
p 1312 976 100 0 0 FDSC:Total elapsed time (utend - utstart)
p 1440 1168 100 0 1 FTVL:DOUBLE
p 1440 1072 100 0 1 PREC:4
p 1440 1136 100 0 1 PV:$(sadtop)$(wfs)
use notes 3568 -313 100 0 notes#13
xform 0 3824 -128
p 4096 -162 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3596 -2 100 0 -1 COMMENT1:This schematic contains the Status records
p 3596 -34 100 0 -1 COMMENT2:describing the timing of an observation
p 3596 -64 100 0 -1 COMMENT3:made by one wavefront sensor.
p 3596 -96 100 0 -1 COMMENT4:It may be duplicated for each wavefront
p 3596 -128 100 0 -1 COMMENT5:sensor, using the wfs macro to distinguish
p 3596 -160 100 0 -1 COMMENT6:each one.
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 2000-03-13 20:46:38 $
p 2576 2320 200 0 -1 id:$Id: wfsTimSad.sch,v 1.2 2000-03-13 20:46:38 cboyer Exp $
p 3120 -432 200 0 -1 project:Gemini Wavefront Sensing System
p 2608 -496 200 0 -1 revision:$Revision: 1.2 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Timing Records
[comments]
