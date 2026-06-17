# python3 ./manthan_input_gen.py ./cnt20y.dqdimacs ./out.qdimacs ./ordering.txt
# cd ../dependencies/manthan
# source ./manthan-venv/bin/activate
# python3 ./manthan.py ../../manthan_test/out.qdimacs --maxrepairitr 30000
# deactivate
# echo "Manthan Done!"
# cp ./out_skolem.v ../../manthan_test/
# cd ../..
# ./bin/main ./manthan_test/cnt20y.dqdimacs ./manthan_test/out_skolem.v ./manthan_test/ordering.txt

#!/bin/bash
# Check if a benchmark file was provided
if [ -z "$1" ]; then
    echo "Error: No benchmark file provided to test.sh"
    exit 1
fi

BENCHMARK_PATH=$1

# 1. Run your input generator script using the dynamic path parameter
python3 ./manthan_input_gen.py "$BENCHMARK_PATH" ./manthan_test/out.qdimacs ./manthan_test/ordering.txt

# 2. Step inside the directory, activate environment, and run Manthan
cd ../dependencies/manthan || exit 1
source ./manthan-venv/bin/activate
python3 ./manthan.py ../../manthan_test/out.qdimacs --maxrepairitr 30000
deactivate

# 3. Copy the synthesized Skolem function back to the workspace
cp ./out_skolem.v ../../manthan_test/
cd ../..

echo "MANTHAN_PREPROCESS_DONE"