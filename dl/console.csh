#!/bin/csh
echo Connecting to $1 console on port $2
xterm -l -geometry 80x50-0+0 -n IOC -T "telnet $1 $2" -e telnet $1 $2
