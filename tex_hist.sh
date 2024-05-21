#!/usr/bin/env sh
PDF_VIEWER="${VIEWER:-evince}"
if [[ -z "$1" ]]; then
    echo "Usage: $0 <history>"
    exit 0
fi

HIST=$1
./util/texhist.py ${HIST} | ./util/tex2standalone.py | xargs $PDF_VIEWER &
