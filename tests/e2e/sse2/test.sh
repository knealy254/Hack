#!/bin/sh
gcc -msse2 movaps.c -o test_movaps
echo movaps
./test_movaps

echo movss
gcc -msse2 movss.c -o test_movss
./test_movss

echo xorps
gcc -msse2 xorps.c -o test_xorps
./test_xorps

echo psrlq
gcc -msse2 psrlq.c -o test_psrlq
./test_psrlq

echo pcmpeqb
gcc -msse2 pcmpeqb.c -o test_pcmpeqb
./test_pcmpeqb

echo pmovmskb
gcc -msse2 pmovmskb.c -o test_pmovmskb
./test_pmovmskb

