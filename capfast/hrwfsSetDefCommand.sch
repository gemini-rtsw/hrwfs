[schematic2]
uniq 63
[tools]
[detail]
s -256 1920 200 0 Set default to detFrameSize CAD
s 2240 -336 500 512 hrwfsSetDefCommand.sch
s -592 2528 500 0 Wavefront Sensing - Set defaults to commands
[cell use]
use egenSubB -96 1031 100 0 initFrameSize
xform 0 48 1456
p -32 992 100 0 1 DESC:Init the detFrameSize CAD record
p 0 1808 100 0 1 FTA:LONG
p 0 1776 100 0 1 FTB:LONG
p 0 1744 100 0 1 FTC:LONG
p 0 1712 100 0 1 FTD:LONG
p 0 1680 100 0 1 FTE:LONG
p 0 1648 100 0 1 FTF:LONG
p 0 1616 100 0 1 FTG:LONG
p 288 1840 100 0 1 FTVA:LONG
p 288 1808 100 0 1 FTVB:LONG
p 288 1776 100 0 1 FTVC:LONG
p 288 1744 100 0 1 FTVD:LONG
p 288 1712 100 0 1 FTVE:LONG
p 288 1680 100 0 1 FTVF:LONG
p 288 1648 100 0 1 FTVG:LONG
p -32 896 100 0 1 PV:$(top)$(wfs)
p -32 960 100 0 1 SCAN:Passive
p -32 928 100 0 1 SNAM:detInitFrameSize
p 416 1840 100 0 1 def(OUTA):$(top)$(wfs)detFrameSize.A
p 416 1808 100 0 1 def(OUTB):$(top)$(wfs)detFrameSize.B
p 416 1776 100 0 1 def(OUTC):$(top)$(wfs)detFrameSize.C
p 416 1744 100 0 1 def(OUTD):$(top)$(wfs)detFrameSize.D
p 416 1712 100 0 1 def(OUTE):$(top)$(wfs)detFrameSize.E
p 416 1680 100 0 1 def(OUTF):$(top)$(wfs)detFrameSize.F
p 416 1648 100 0 1 def(OUTG):$(top)$(wfs)detFrameSize.G
use bd200tr -1136 -584 -100 0 frame
xform 0 1504 1120
p 2496 -352 200 0 1 author:C. Boyer
p 3008 -384 100 0 0 border:D
p 2496 -432 200 0 1 checked:
p 2992 -448 200 0 -1 date:
p 2480 2656 200 0 -1 id:
p 3008 -96 200 0 -1 project:Gemini Wavefront Sensing System
p 2496 -176 200 0 -1 revision:
p 3008 -224 200 0 -1 title:Set defaults to commands
[comments]
