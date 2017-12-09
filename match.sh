#!/bin/bash

e2=$1
tc=$2
mg=$3

if [ -z "$e2" ]; then
    e2=fairymax
fi

if [ -z "$tc" ]; then
    tc=2
fi

if [ -z "$mg" ]; then
    mg=4
fi

dt=$(date "+%F-%T" | sed -e "s/://g")
sf=$(printf "$e2-%s.pgn" "$dt")

xboard -fUCI -fcp ./sbchess.exe -scp $e2 -tc $tc -mg $mg -sgf $sf -inc 0
