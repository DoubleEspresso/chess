#!/bin/sh

#cd ..
#git checkout testing
#make clean
#make
#mv sbchess.exe regression/testing.exe

#git checkout master
#make clean
#make
#cp sbchess.exe regression/master.exe

if [ -f "results.pgn" ]; then
    rm results.pgn
fi

./cutechess-cli -maxmoves 45 -pgnout results.pgn -repeat -games 1000 -srand 2653263383 -resign movecount=1 score=300 -draw movenumber=24 movecount=8 score=20 -concurrency 6 -openings file="2moves_v1.pgn" format=pgn order=random plies=16 -engine name=testing cmd=testing.exe -engine name=master cmd=master.exe -each proto=uci tc=1+0.1
