[schematic2]
uniq 129
[tools]
[detail]
w 1986 811 100 0 n#127 egenSub.combState.VALA 1856 800 2176 800 esirs.state.INP
w 1938 75 100 0 n#125 egenSub.combState.FLNK 1856 64 2080 64 2080 640 2176 640 esirs.state.SLNK
w 1298 -149 100 0 n#120 ewait.waitState.FLNK 1248 -160 1408 -160 1408 96 junction
w 1154 811 100 0 n#120 esirs.controlState.FLNK 960 800 1408 800 1408 96 1568 96 egenSub.combState.SLNK
w 1170 779 100 0 n#119 esirs.controlState.VAL 960 768 1440 768 1440 640 1568 640 egenSub.combState.INPC
w -318 619 100 0 n#107 ecalcs.counter.VAL -384 608 -192 608 -192 800 0 800 esirs.heartBeat.INP
w -510 875 100 0 n#107 ecalcs.counter.INPA -672 800 -768 800 -768 864 -192 864 -192 800 junction
w -222 651 100 0 n#106 ecalcs.counter.FLNK -384 640 0 640 esirs.heartBeat.SLNK
s -160 2208 500 0 Wavefront Sensing - System Status Records
s 2512 -704 500 512 systemSad.sch
[cell use]
use systemHistorySad 3200 -249 100 0 systemHistorySad#128
xform 0 3296 -128
use ewait 544 -249 100 0 waitState
xform 0 896 80
p 867 328 100 0 1 CALC:A
p 656 272 100 0 1 DESC:Monitor component controller state
p 672 -96 100 0 1 OOPT:On Change
p 672 30 100 0 1 PRIO:LOW
p 928 16 100 0 1 PV:$(sadtop)
p 672 222 100 0 1 SCAN:I/O Intr
p 672 -192 100 0 1 def(INAN):$(agtop)cc:state.VAL
use esirs 544 551 100 0 controlState
xform 0 752 704
p 608 512 100 0 1 DESC:Sequencer controller state
p 608 480 100 0 1 FTVL:STRING
p 608 448 100 0 1 PV:$(sadtop)
p 864 480 100 0 1 SCAN:Passive
p 864 448 100 0 1 SNAM:
use esirs -672 1159 100 0 name
xform 0 -464 1312
p -608 1120 100 0 1 DESC:System name
p -736 896 100 0 0 FDSC:System name
p -608 1088 100 0 1 FTVL:STRING
p -608 1024 100 0 1 PV:$(sadtop)
p -608 992 100 0 1 SCAN:Passive
p -608 1056 100 0 1 SNAM:
use esirs 2176 551 100 0 state
xform 0 2384 704
p 2240 512 100 0 1 DESC:Overall system state
p 2112 288 100 0 0 FDSC:System state [BOOTING|INITIALISING|RUNNING]
p 2240 480 100 0 1 FTVL:STRING
p 2240 416 100 0 1 PV:$(sadtop)
p 2240 448 100 0 1 SNAM:
p 2240 384 100 0 1 VAL:BOOTING
use esirs 0 1159 100 0 version
xform 0 208 1312
p 64 1120 100 0 1 DESC:System version number
p -64 896 100 0 0 FDSC:System version number
p 64 1088 100 0 1 FTVL:STRING
p 64 1024 100 0 1 PV:$(sadtop)
p 64 1056 100 0 1 SNAM:
use esirs 640 1159 100 0 trackId
xform 0 848 1312
p 704 1120 100 0 1 DESC:Data stream ID
p 704 1056 100 0 1 EGU:0/1
p 576 896 100 0 0 FDSC:Data stream ID
p 704 1088 100 0 1 FTVL:LONG
p 864 1024 100 0 1 HIGH:100000000
p 864 992 100 0 1 HIHI:100000000
p 704 992 100 0 1 LOLO:0
p 704 1024 100 0 1 LOW:0
p 704 960 100 0 1 PV:$(sadtop)
use esirs 1248 1159 100 0 arrayS
xform 0 1456 1312
p 1312 1120 100 0 1 DESC:Status of continuous data
p 1184 896 100 0 0 FDSC:Array status [GOOD|LATE|INVALID]
p 1312 1088 100 0 1 FTVL:STRING
p 1312 1024 100 0 1 PV:$(sadtop)
p 1312 1056 100 0 1 SNAM:
p 1312 992 100 0 1 VAL:BOOTING
use esirs 1856 1159 100 0 seeing
xform 0 2064 1312
p 1920 1120 100 0 1 DESC:Current seeing estimate
p 1920 1056 100 0 1 EGU:arcseconds
p 1792 896 100 0 0 FDSC:Current seeing estimate
p 1920 1088 100 0 1 FTVL:DOUBLE
p 2080 1024 100 0 1 HIGH:3.0
p 2080 992 100 0 1 HIHI:10.0
p 1920 992 100 0 1 LOLO:0.0
p 1920 1024 100 0 1 LOW:0.0
p 1920 960 100 0 1 PV:$(sadtop)
use esirs 2464 1159 100 0 testResults
xform 0 2672 1312
p 2528 1120 100 0 1 DESC:Test results
p 2560 1120 100 0 0 FDSC:Test results
p 2528 1088 100 0 1 FTVL:STRING
p 2528 1024 100 0 1 PV:$(sadtop)
p 2528 1056 100 0 1 SNAM:
use esirs 0 551 100 0 heartBeat
xform 0 208 704
p 64 512 100 0 1 DESC:Heartbeat
p -64 320 100 0 0 EVNT:0
p -64 288 100 0 0 FDSC:
p 64 480 100 0 1 FTVL:LONG
p 64 416 100 0 1 PV:$(sadtop)
p 64 448 100 0 1 SNAM:
use esirs 3072 1159 100 0 obsMode
xform 0 3280 1312
p 3136 1120 100 0 1 DESC:Observing mode
p 3168 1120 100 0 0 FDSC:Observing mode (STARE/MOVIE)
p 3136 1088 100 0 1 FTVL:STRING
p 3136 1024 100 0 1 PV:$(sadtop)
p 3136 1056 100 0 1 SNAM:
use esirs 3680 1159 100 0 obsType
xform 0 3888 1312
p 3744 1120 100 0 1 DESC:Observation type
p 3776 1120 100 0 0 FDSC:(DARK/FLAT/ZERO/OBJECT/UNDEFINED)
p 3744 1088 100 0 1 FTVL:STRING
p 3744 1024 100 0 1 PV:$(sadtop)
p 3744 1056 100 0 1 SNAM:
use egenSub 1568 7 100 0 combState
xform 0 1712 432
p 1552 848 100 0 1 DESC:Combine states from DC and CC
p 1664 768 100 0 1 FTA:STRING
p 1664 704 100 0 1 FTB:STRING
p 1664 640 100 0 1 FTC:STRING
p 1648 96 100 0 1 FTVA:STRING
p 1632 -144 100 0 1 PV:$(sadtop)
p 1632 -16 100 0 1 SCAN:Passive
p 1632 -48 100 0 1 SNAM:cicsStateCombine
p 1632 -80 100 0 1 def(INPA):$(agtop)cc:state
p 1632 -112 100 0 1 def(INPB):$(sadtop)dc:state
use ecalcs -672 327 100 0 counter
xform 0 -528 592
p -608 288 100 0 1 CALC:A+1
p -608 224 100 0 1 PV:$(sadtop)
p -608 256 100 0 1 SCAN:1 second
use resourceSad 2880 -249 100 0 resourceSad#78
xform 0 2976 -128
use systemHealthSad 2560 -249 100 0 systemHealthSad#48
xform 0 2656 -128
use bd200tr -1024 -904 -100 0 frame
xform 0 1616 800
p 2608 -672 200 0 1 author:S.M.Beard
p 3120 -704 100 0 0 border:D
p 2608 -752 200 0 1 checked:B.Goodrich
p 3184 -688 200 0 -1 date:$Date: 2000-03-13 20:46:38 $
p 2592 2336 200 0 -1 id:$Id: systemSad.sch,v 1.3 2000-03-13 20:46:38 cboyer Exp $
p 3120 -416 200 0 -1 project:Gemini Wavefront Sensing System
p 2592 -480 200 0 -1 revision:$Revision: 1.3 $
p 3120 -544 200 0 -1 title:System Status Records
[comments]
