[schematic2]
uniq 135
[tools]
[detail]
w 3378 1227 100 0 n#134 egenSubC.aoE.FLNK 3328 1216 3488 1216 3488 1248 3584 1248 egenSubC.aoZ.SLNK
w 2786 1227 100 0 n#133 egenSub.ao.FLNK 2688 1216 2944 1216 2944 1248 3040 1248 egenSubC.aoE.SLNK
w 3154 2059 100 0 n#131 egenSub.ao.OUTA 2688 1920 2880 1920 2880 2048 3488 2048 3488 1952 3584 1952 egenSubC.aoZ.A
w 2786 1867 100 0 n#130 egenSub.ao.OUTB 2688 1856 2944 1856 2944 1952 3040 1952 egenSubC.aoE.A
s 128 2176 500 0 Wavefront Sensing - WFS Status Records
s 2464 -704 500 512 wfsSadGensub.sch
[cell use]
use egenSubC 3584 1159 100 0 aoZ
xform 0 3728 1584
p 3648 1104 100 0 1 DESC:Display AO Zernike values
p 3648 1072 100 0 1 INAM:
p 3648 944 100 0 1 NOA:19
p 3296 1710 100 0 0 PREC:4
p 3648 1008 100 0 1 PV:$(top)$(wfs)
p 3648 976 100 0 1 SCAN:Passive
p 3648 1040 100 0 1 SNAM:gensubFanDoubles
use egenSubC 3040 1159 100 0 aoE
xform 0 3184 1584
p 3104 1104 100 0 1 DESC:Display AO Error values
p 3104 1072 100 0 1 INAM:
p 3104 944 100 0 1 NOA:19
p 2752 1710 100 0 0 PREC:4
p 3104 1008 100 0 1 PV:$(top)$(wfs)
p 3104 976 100 0 1 SCAN:Passive
p 3104 1040 100 0 1 SNAM:gensubFanDoubles
use egenSub 1824 1159 100 0 ttf
xform 0 1968 1584
p 1888 1104 100 0 1 DESC:Time averaged T-T-F data
p 1904 1424 100 0 1 FTJ:DOUBLE
p 1904 1392 100 0 1 FTVJ:DOUBLE
p 1888 1072 100 0 1 INAM:gensubToTcsInit
p 1904 1360 100 0 1 NOJ:8
p 1904 1328 100 0 1 NOVJ:8
p 1536 1710 100 0 0 PREC:2
p 1888 1008 100 0 1 PV:$(sadtop)$(wfs)
p 1888 976 100 0 1 SCAN:.1 second
p 1888 1040 100 0 1 SNAM:gensubToTcsTtf
p 1776 1930 75 0 -1 pproc(INPA):NPP
p 2112 1354 75 0 -1 pproc(OUTJ):NPP
use egenSub 2400 1159 100 0 ao
xform 0 2544 1584
p 2464 1104 100 0 1 DESC:Active optics data
p 2480 1424 100 0 1 FTJ:DOUBLE
p 2480 1936 100 0 1 FTVA:DOUBLE
p 2480 1856 100 0 1 FTVB:DOUBLE
p 2480 1392 100 0 1 FTVJ:DOUBLE
p 2464 1072 100 0 1 INAM:gensubToTcsInit
p 2480 1360 100 0 1 NOJ:40
p 2480 1904 100 0 1 NOVA:19
p 2480 1824 100 0 1 NOVB:19
p 2480 1328 100 0 1 NOVJ:40
p 2112 1710 100 0 0 PREC:2
p 2464 1008 100 0 1 PV:$(sadtop)$(wfs)
p 2464 976 100 0 1 SCAN:.1 second
p 2464 1040 100 0 1 SNAM:gensubToTcsAo
p 2688 1354 75 0 -1 pproc(OUTJ):NPP
use notes 2816 -153 100 0 notes#13
xform 0 3072 32
p 3344 -2 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 2844 158 100 0 -1 COMMENT1:This schematic contains the Status records
p 2844 126 100 0 -1 COMMENT2:connected with one wavefront sensor.
p 2844 96 100 0 -1 COMMENT3:It may be duplicated for each wavefront
p 2844 64 100 0 -1 COMMENT4:sensor, using the wfs macro to distinguish
p 2844 32 100 0 -1 COMMENT5:each one.
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:S.M.Beard
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:$Date: 1999-03-17 03:14:14 $
p 2576 2320 200 0 -1 id:$Id: wfsSadGensub.sch,v 1.1.1.1 1999-03-17 03:14:14 cboyer Exp $
p 3120 -432 200 0 -1 project:Gemini Wavefront Sensing System
p 2608 -496 200 0 -1 revision:$Revision: 1.1.1.1 $
p 3120 -560 200 0 -1 title:Wavefront Sensor Status Records
[comments]
