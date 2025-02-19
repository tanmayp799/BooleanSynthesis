#!/bin/bash

# Create the output directory if it doesn't exist
mkdir -p ./data/afterhqspre

# Iterate over all .dqdimacs files in the dqbf directory
for file in ./benchmarks/dqbf/*.dqdimacs; do
    # Extract the filename without the path
    filename=$(basename "$file")

    output_file="./data/afterhqspre/$filename"
    
    # Create an empty output file
    touch "$output_file"

    echo "Processing: $filename"
    # Run hqspre with the required arguments
    ./hqspre -o "./data/afterhqspre/$filename" --dqbf yes "$file"
done

echo "Processing complete. Output saved in afterhqspre/"
