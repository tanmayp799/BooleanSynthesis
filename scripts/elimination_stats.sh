#!/bin/bash

TARGET_DIR="./benchmark_tests/afterhqspre"

# Check if the benchmark directory exists
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' not found!"
    exit 1
fi

# Iterate recursively over each .dqdimacs file in the folder and its subfolders
find "$TARGET_DIR" -type f -name "*.dqdimacs" -print0 | while IFS= read -r -d '' file; do
    echo "Running ./bin/main on: $file"
    ./bin/main "$file" > out.log 2> out.errlog
done
