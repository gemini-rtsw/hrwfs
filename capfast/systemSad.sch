[schematic2]
uniq 110
[tools]
[detail]
w -510 875 100 0 n#107 ecalcs.counter.INPA -672 800 -768 800 -768 864 -192 864 -192 800 junction
w -318 619 100 0 n#107 ecalcs.counter.VAL -384 608 -192 608 -192 800 0 800 esirs.heartBeat.INP
w -222 651 100 0 n#106 ecalcs.counter.FLNK -384 640 0 640 esirs.heartBeat.SLNK
s 2512 -704 500 512 systemSad.sch
s -160 2208 500 0 Wavefront Sensing - System Status Records
[cell use]
use esirs 3680 1159 100 0 obsType
xform 0 3888 1312
p 3744 1120 100 0 1 DESC:Observation type
p 3776 1120 100 0 0 FDSC:(DARK/FLAT/ZERO/OBJECT/UNDEFINED)
p 3744 1088 100 0 1 FTVL:STRING
p 3744 1024 100 0 1 PV:$(sadtop)
p 3744 1056 100 0 1 SNAM:
use esirs 3072 1159 100 0 obsMode
xform 0 3280 1312
p 3136 1120 100 0 1 DESC:Observing mode
p 3168 1120 100 0 0 FDSC:Observing mode (STARE/MOVIE)
p 3136 1088 100 0 1 FTVL:STRING
p 3136 1024 100 0 1 PV:$(sadtop)
p 3136 1056 100 0 1 SNAM:
use esirs 0 551 100 0 heartBeat
xform 0 208 704
p 64 512 100 0 1 DESC:Heartbeat
p -64 320 100 0 0 EVNT:0
p -64 288 100 0 0 FDSC:
p 64 480 100 0 1 FTVL:LONG
p 64 416 100 0 1 PV:$(sadtop)
p 64 448 100 0 1 SNAM:
use esirs 2464 1159 100 0 testResults
xform 0 2672 1312
p 2528 1120 100 0 1 DESC:Test results
p 2560 1120 100 0 0 FDSC:Test results
p 2528 1088 100 0 1 FTVL:STRING
p 2528 1024 100 0 1 PV:$(sadtop)
p 2528 1056 100 0 1 SNAM:
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
use esirs 1248 1159 100 0 arrayS
xform 0 1456 1312
p 1312 1120 100 0 1 DESC:Status of continuous data
p 1184 896 100 0 0 FDSC:Array status [GOOD|LATE|INVALID]
p 1312 1088 100 0 1 FTVL:STRING
p 1312 1024 100 0 1 PV:$(sadtop)
p 1312 1056 100 0 1 SNAM:
p 1312 992 100 0 1 VAL:BOOTING
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
use esirs 0 1159 100 0 debugMode
xform 0 208 1312
p 64 1120 100 0 1 DESC:Debugging mode
p -64 928 100 0 0 EVNT:0
p -64 896 100 0 0 FDSC:Debugging mode [NONE|MIN|FULL]
p 64 1088 100 0 1 FTVL:STRING
p 64 1024 100 0 1 PV:$(sadtop)
p 64 1056 100 0 1 SNAM:
use esirs -672 1159 100 0 simMode
xform 0 -464 1312
p -608 1120 100 0 1 DESC:Simulation mode
p -736 928 100 0 0 EVNT:0
p -736 896 100 0 0 FDSC:Simulation mode [VSM|FAST|FULL|NONE]
p -608 1088 100 0 1 FTVL:STRING
p -608 1024 100 0 1 PV:$(sadtop)
p -608 1056 100 0 1 SNAM:
use esirs 640 1767 100 0 version
xform 0 848 1920
p 704 1728 100 0 1 DESC:System version number
p 576 1504 100 0 0 FDSC:System version number
p 704 1696 100 0 1 FTVL:STRING
p 704 1632 100 0 1 PV:$(sadtop)
p 704 1664 100 0 1 SNAM:
use esirs 0 1767 100 0 state
xform 0 208 1920
p 64 1728 100 0 1 DESC:System state
p -64 1504 100 0 0 FDSC:System state [BOOTING|INITIALISING|RUNNING]
p 64 1696 100 0 1 FTVL:STRING
p 64 1632 100 0 1 PV:$(sadtop)
p 64 1664 100 0 1 SNAM:
p 64 1600 100 0 1 VAL:BOOTING
use esirs -672 1767 100 0 name
xform 0 -464 1920
p -608 1728 100 0 1 DESC:System name
p -736 1504 100 0 0 FDSC:System name
p -608 1696 100 0 1 FTVL:STRING
p -608 1632 100 0 1 PV:$(sadtop)
p -608 1600 100 0 1 SCAN:Passive
p -608 1664 100 0 1 SNAM:
use ecalcs -672 327 100 0 counter
xform 0 -528 592
p -608 288 100 0 1 CALC:A+1
p -608 224 100 0 1 PV:$(sadtop)
p -608 256 100 0 1 SCAN:1 second
use errorLogSad 1920 -377 100 0 errorLogSad#79
xform 0 2016 -256
use resourceSad 1600 -377 100 0 resourceSad#78
xform 0 1696 -256
use notes 3520 -265 100 0 notes#13
xform 0 3776 -80
p 4048 -114 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3548 46 100 0 -1 COMMENT1:This schematic contains the systemwide
p 3548 14 100 0 -1 COMMENT2:status records.
p 3548 -48 100 0 -1 COMMENT4:NOTE: The resource usage records assume
p 3548 -80 100 0 -1 COMMENT5:there are between 1 and 3 processors.
p 3548 -112 100 0 -1 COMMENT6:It would be more flexible to put each
p 3548 -144 100 0 -1 COMMENT7:set of these records in a separate
p 3548 -176 100 0 -1 COMMENT8:"procSad.sch" schematic. SMB - 4/2/98.
use systemHealthSad 1280 -377 100 0 systemHealthSad#48
xform 0 1376 -256
use bd200tr -1024 -904 -100 0 frame
xform 0 1616 800
p 2608 -672 200 0 1 author:S.M.Beard
p 3120 -704 100 0 0 border:D
p 2608 -752 200 0 1 checked:B.Goodrich
p 3184 -688 200 0 -1 date:$Date: 2000-02-03 01:17:55 $
p 2592 2336 200 0 -1 id:$Id: systemSad.sch,v 1.2 2000-02-03 01:17:55 cboyer Exp $
p 3120 -416 200 0 -1 project:Gemini Wavefront Sensing System
p 2592 -480 200 0 -1 revision:$Revision: 1.2 $
p 3120 -544 200 0 -1 title:System Status Records
[comments]
