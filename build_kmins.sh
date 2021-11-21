# test.txt is suspicious-document00001.txt for src folder, use the first 100 for test
rm -f no_multiple_kmins
g++ -O3 -std=c++11 -o no_multiple_kmins txtalign_no_multi_kmins.cc
