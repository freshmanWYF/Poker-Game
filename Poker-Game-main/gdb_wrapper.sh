#!/bin/bash
# gdb exec-wrapper: sets environment, then execs the actual program
export LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libpthread.so.0
export QT_QPA_PLATFORM=xcb
export DISPLAY="${DISPLAY:-:0}"
exec "$@"
