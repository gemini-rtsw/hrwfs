[schematic2]
uniq 100
[tools]
[detail]
s -368 2176 500 0 Wavefront Sensing - Data Header Status Records
s 2464 -704 500 512 wfsDataSad.sch
[cell use]
use esirs -416 1191 100 0 nexp
xform 0 -208 1344
p -352 1152 100 0 1 DESC:Actual exposures per data set
p -480 928 100 0 0 FDSC:Actual exposures per data set
p -352 1120 100 0 1 FTVL:LONG
p -352 1088 100 0 1 PV:$(sadtop)$(wfs)
use esirs -416 711 100 0 nframes
xform 0 -208 864
p -352 672 100 0 1 DESC:Frames per data set
p -480 448 100 0 0 FDSC:Frames per data set (-1 for continuous)
p -352 640 100 0 1 FTVL:LONG
p -352 608 100 0 1 PV:$(sadtop)$(wfs)
use esirs 304 711 100 0 bunit
xform 0 512 864
p 368 672 100 0 1 DESC:Data units
p 240 448 100 0 0 FDSC:Data units
p 368 640 100 0 1 FTVL:STRING
p 368 608 100 0 1 PV:$(sadtop)$(wfs)
p 368 576 100 0 1 VAL:SDSU units
use esirs -416 1671 100 0 nexpRQ
xform 0 -208 1824
p -352 1632 100 0 1 DESC:Requested exposures per data set
p -480 1408 100 0 0 FDSC:Requested exposures per data set
p -352 1600 100 0 1 FTVL:LONG
p -352 1568 100 0 1 PV:$(sadtop)$(wfs)
use esirs 288 1671 100 0 dataLabel
xform 0 496 1824
p 352 1632 100 0 1 DESC:DHS data label
p 224 1408 100 0 0 FDSC:DHS data label
p 352 1600 100 0 1 FTVL:STRING
p 352 1568 100 0 1 PV:$(sadtop)$(wfs)
use notes 3568 -313 100 0 notes#13
xform 0 3824 -128
p 4096 -162 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3596 -2 100 0 -1 COMMENT1:This schematic contains the Status records
p 3596 -34 100 0 -1 COMMENT2:connected with the data header of
p 3596 -64 100 0 -1 COMMENT3:one wavefront sensor.
p 3596 -96 100 0 -1 COMMENT4:It may be duplicated for each wavefront
p 3596 -128 100 0 -1 COMMENT5:sensor, using the wfs macro to distinguish
p 3596 -160 100 0 -1 COMMENT6:each one.
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 1999-03-17 03:14:13 $
p 2576 2320 200 0 -1 id:$Id: wfsDataSad.sch,v 1.1.1.1 1999-03-17 03:14:13 cboyer Exp $
p 3120 -432 200 0 -1 project:Gemini Wavefront Sensing System
p 2608 -496 200 0 -1 revision:$Revision: 1.1.1.1 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Data Header Records
[comments]
