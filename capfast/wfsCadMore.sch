[schematic2]
uniq 125
[tools]
[detail]
s 336 2240 500 0 Wavefront Sensing - WFS CAD Records
s 2464 -704 500 512 wfsCadMore.sch
[cell use]
use ecad2 160 1447 100 0 detDhsDisplay
xform 0 320 1760
p 224 1392 100 0 1 DESC:Set dhs display
p 272 1824 100 0 1 FTVA:LONG
p 272 1760 100 0 0 FTVB:STRING
p 224 1360 100 0 1 INAM:epToVxCadInit
p 224 1296 100 0 1 PV:$(top)$(wfs)
p 224 1328 100 0 1 SNAM:epToVxCadExecute
use ecad2 -512 1447 100 0 detDhsReconnect
xform 0 -352 1760
p -448 1392 100 0 1 DESC:Set dhs connection
p -400 1824 100 0 1 FTVA:LONG
p -400 1760 100 0 0 FTVB:STRING
p -448 1360 100 0 1 INAM:epToVxCadInit
p -448 1296 100 0 1 PV:$(top)$(wfs)
p -448 1328 100 0 1 SNAM:epToVxCadExecute
use bd200tr -1024 -920 -100 0 frame
xform 0 1616 784
p 2608 -688 200 0 1 author:C. Boyer
p 3120 -720 100 0 0 border:D
p 2608 -768 200 0 1 checked:B.Goodrich
p 3120 -784 200 0 -1 date:2001/02/16
p 2592 2304 200 0 -1 id:
p 3120 -432 200 0 -1 project:Gemini Wavefront Sensing System
p 2592 -528 200 0 -1 revision:$Revision: 1.1 $
p 3120 -560 200 0 -1 title:Wavefront Sensor CAD Records (more)
[comments]
