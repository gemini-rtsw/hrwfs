#!/bin/csh
echo Connecting to $1
xterm -l -geometry 80x50-30+30 -n `echo $1 | awk -F. '{print $1}'` -title "telnet: $1" -e telnet $1
