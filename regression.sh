#!/bin/sh

git checkout testing
make clean
make
mv hedwig.exe /home/fishtest/fishtest/worker/testing/base.exe

git checkout master
make clean
make
cp hedwig.exe /home/fishtest/fishtest/worker/testing

cd /home/fishtest/fishtest/worker/testing

sudo ./cutechess-cli -repeat -rounds 971 -tournament gauntlet -pgnout results.pgn -srand 2653263383 -resign movecount=3 score=400 -draw movenumber=34 movecount=8 score=20 -concurrency 5 -openings file="2moves_v1.pgn" format=pgn order=random plies=16 -engine name=hedwig cmd=hedwig.exe -engine name=base cmd=base.exe -each proto=uci tc=3.08+0.03

