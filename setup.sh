sudo apt-get update 
sudo apt-get install -y libreadline-dev libncurses5-dev libboost-program-options-dev gv
git submodule update --init
cd dependencies/abc
make libabc.a -j $(nproc)