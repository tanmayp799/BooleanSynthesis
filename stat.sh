#!/bin/bash

for testfile in ./data/afterhqspre/*; do
    # Check if the file has at least one line starting with 'd '
    if grep -q '^d ' "$testfile"; then
        echo "Running test: $testfile"
        ./bin/main "$testfile"
    else
        echo "Skipping $testfile (no line starting with 'd ')"
    fi
done
