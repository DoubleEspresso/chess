#!/bin/sh

cd ..
git checkout testing
make clean
make
mv sbchess.exe regression/testing.exe

git checkout master
make clean
make
cp sbchess.exe regression/master.exe

cd regression

./cutechess-cli -repeat -rounds 1000 -tournament gauntlet -pgnout results.pgn -srand 2653263383 -resign movecount=3 score=400 -draw movenumber=34 movecount=8 score=20 -concurrency 5 -openings file="2moves_v1.pgn" format=pgn order=random plies=16 -engine name=testing cmd=testing.exe -engine name=master cmd=master.exe -each proto=uci tc=20+0.1

