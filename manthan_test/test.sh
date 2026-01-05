python3 ./manthan_input_gen.py ./manthan_test.dqdimacs ./out.qdimacs ./ordering.txt
cd ../dependencies/manthan
source ./manthan-venv/bin/activate
python3 ./manthan.py ../../manthan_test/out.qdimacs
deactivate
cp ./out_skolem.v ../../manthan_test/
cd ../..
./bin/main ./manthan_test/manthan_test.dqdimacs ./manthan_test/out_skolem.v ./manthan_test/ordering.txt