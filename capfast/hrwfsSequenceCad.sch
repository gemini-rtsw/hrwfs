[schematic2]
uniq 173
[tools]
[detail]
w 802 1931 100 0 n#172 ecad2.test.STLK 768 1920 896 1920 896 1984 960 1984 testFanCad.testFanCad#171.STLK
w 1938 1931 100 0 n#170 ecad2.park.STLK 1920 1920 2016 1920 2016 1984 2112 1984 parkFanCad.parkFanCad#169.STLK
w 3154 1931 100 0 n#168 ecad2.datum.STLK 3136 1920 3232 1920 3232 1984 3328 1984 datumFanCad.datumFanCad#167.STLK
w -542 1931 100 0 n#166 ecad2.init.STLK -576 1920 -448 1920 -448 1984 -384 1984 initFanCad.initFanCad#165.STLK
s -768 2560 500 0 Wavefromt Sensing System - Sequence CAD Records
s 2240 -336 500 512 hrwfsSequenceCad.sch
[cell use]
use testFanCad 912 1847 100 0 testFanCad#171
xform 0 1056 1984
p 960 1824 100 0 1 set1:cccad $(agtop)cc:test
use parkFanCad 2080 1847 100 0 parkFanCad#169
xform 0 2208 1984
p 2112 1824 100 0 1 set1:cccad $(agtop)cc:park
use datumFanCad 3232 1815 100 0 datumFanCad#167
xform 0 3424 1984
p 3328 1824 100 0 1 set1:cccad $(agtop)cc:datum
use initFanCad -368 1863 100 0 initFanCad#165
xform 0 -288 1984
p -384 1824 100 0 1 set1:cccad $(agtop)cc:init
use ecad2 1600 999 100 0 endObserve
xform 0 1760 1312
p 1664 960 100 0 1 DESC:endObserve
p 1712 1376 100 0 0 FTVA:STRING
p 1712 1312 100 0 0 FTVB:STRING
p 1664 928 100 0 1 INAM:epToVxCadInit
p 1664 864 100 0 1 PV:$(top)
p 1664 896 100 0 1 SNAM:epToVxCadExecute
use ecad2 448 167 100 0 guide
xform 0 608 480
p 512 128 100 0 1 DESC:Guide
p 560 544 100 0 0 FTVA:STRING
p 560 480 100 0 0 FTVB:STRING
p 512 96 100 0 1 INAM:epToVxCadInit
p 512 32 100 0 1 PV:$(top)
p 512 64 100 0 1 SNAM:epToVxCadExecute
use ecad2 -896 167 100 0 endVerify
xform 0 -736 480
p -832 128 100 0 1 DESC:EndVerify
p -784 544 100 0 0 FTVA:STRING
p -784 480 100 0 0 FTVB:STRING
p -832 96 100 0 1 INAM:epToVxCadInit
p -832 32 100 0 1 PV:$(top)
p -832 64 100 0 1 SNAM:epToVxCadExecute
use ecad2 448 999 100 0 verify
xform 0 608 1312
p 512 960 100 0 1 DESC:Verify
p 560 1376 100 0 0 FTVA:STRING
p 560 1312 100 0 0 FTVB:STRING
p 512 928 100 0 1 INAM:epToVxCadInit
p 512 864 100 0 1 PV:$(top)
p 512 896 100 0 1 SNAM:epToVxCadExecute
use ecad2 1600 1831 100 0 park
xform 0 1760 2144
p 1664 1792 100 0 1 DESC:Prepare for shutdown
p 1712 2208 100 0 0 FTVA:STRING
p 1712 2144 100 0 0 FTVB:STRING
p 1664 1760 100 0 1 INAM:epToVxCadInit
p 1664 1696 100 0 1 PV:$(top)
p 1664 1728 100 0 1 SNAM:epToVxCadExecute
use ecad2 -896 999 100 0 reboot
xform 0 -736 1312
p -832 960 100 0 1 DESC:Reboot system
p -784 1376 100 0 1 FTVA:STRING
p -784 1312 100 0 1 FTVB:STRING
p -832 928 100 0 1 INAM:epToVxCadInit
p -832 864 100 0 1 PV:$(top)
p -832 896 100 0 1 SNAM:epToVxCadExecute
use ecad2 448 1831 100 0 test
xform 0 608 2144
p 512 1792 100 0 1 DESC:Self test
p 560 2208 100 0 1 FTVA:LONG
p 560 2144 100 0 1 FTVB:LONG
p 512 1760 100 0 1 INAM:epToVxCadInit
p 512 1696 100 0 1 PV:$(top)
p 512 1728 100 0 1 SNAM:epToVxCadExecute
use ecad2 -896 1831 100 0 init
xform 0 -736 2144
p -832 1792 100 0 1 DESC:Initialise
p -784 2208 100 0 0 FTVA:STRING
p -784 2144 100 0 0 FTVB:STRING
p -832 1760 100 0 1 INAM:epToVxCadInit
p -832 1696 100 0 1 PV:$(top)
p -832 1728 100 0 1 SNAM:epToVxCadExecute
use ecad2 2816 1831 100 0 datum
xform 0 2976 2144
p 2880 1792 100 0 1 DESC:Datum
p 2928 2208 100 0 0 FTVA:STRING
p 2928 2144 100 0 0 FTVB:STRING
p 2880 1760 100 0 1 INAM:epToVxCadInit
p 2880 1696 100 0 1 PV:$(top)
p 2880 1728 100 0 1 SNAM:epToVxCadExecute
use ecad2 2816 999 100 0 endGuide
xform 0 2976 1312
p 2880 960 100 0 1 DESC:endGuide
p 2928 1376 100 0 0 FTVA:STRING
p 2928 1312 100 0 0 FTVB:STRING
p 2880 928 100 0 1 INAM:epToVxCadInit
p 2880 864 100 0 1 PV:$(top)
p 2880 896 100 0 1 SNAM:epToVxCadExecute
use notes 3456 55 100 0 notes#13
xform 0 3712 240
p 3984 206 100 0 0 AUTHOR:S.M.Beard and N.Dillon
p 3484 366 100 0 -1 COMMENT1:This schematic contains the CAD records
p 3484 334 100 0 -1 COMMENT2:for AGWPS sequence commands.
p 3484 304 100 0 -1 COMMENT3:.
p 3484 272 100 0 -1 COMMENT4:See ICD 1.6.2/1.6.3 for a description
p 3484 240 100 0 -1 COMMENT5:of what these commands do.
p 3484 208 100 0 -1 COMMENT6:.
use bd200tr -1136 -584 -100 0 frame
xform 0 1504 1120
p 2496 -352 200 0 1 author:S.M.Beard
p 3008 -384 100 0 0 border:D
p 2496 -432 200 0 1 checked:A.Foster
p 2992 -448 200 0 -1 date:$Date: 2000-03-13 20:46:37 $
p 2480 2656 200 0 -1 id:$Id: hrwfsSequenceCad.sch,v 1.4 2000-03-13 20:46:37 cboyer Exp $
p 3008 -96 200 0 -1 project:Gemini Wavefront Sensing System
p 2496 -176 200 0 -1 revision:$Revision: 1.4 $
p 3008 -224 200 0 -1 title:Sequence CAD Records
[comments]
