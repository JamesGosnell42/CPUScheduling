#!/bin/sh

algo="$1"

case "$algo" in
FCFS|SJF|SRT|RR) ;;
*) echo "USAGE: $0 ALGO"
echo "ALGO may be FCFS, SJF, SRT, or RR."
exit 1
;;
esac

awk "BEGIN { ok = 0 }
/started for $algo/ { ok = 1 }
ok == 1 {print}
/ended for $algo/ {ok = 0}"
