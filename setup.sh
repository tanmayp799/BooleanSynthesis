sudo apt-get update 
sudo apt-get install -y libreadline-dev libncurses5-dev libboost-program-options-dev gv
git submodule update --init --recursive
cd dependencies/abc
make clean
make libabc.a -j $(nproc) ABC_USE_NO_READLINE=1
cd ../cadical
./configure && make
cd ../manthan