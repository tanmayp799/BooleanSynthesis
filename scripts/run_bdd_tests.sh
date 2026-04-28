#!/bin/bash

TARGET_DIR="./benchmark_tests/afterhqspre/CSP/"
LOG_DIR="./logs/bdd_tests"
STATS_DIR="./statistics"
STATS_FILE="$STATS_DIR/BDD_timeout.csv"

# Timeout duration: 1 hour (3600 seconds)
TIMEOUT_LIMIT=3600

# Ensure log and statistics directories exist
mkdir -p "$LOG_DIR"
mkdir -p "$STATS_DIR"

# Initialize the CSV file with a header
echo "benchmark,time(s),result" > "$STATS_FILE"

# Check if the target directory exists
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' not found!"
    exit 1
fi

# Iterate recursively over each .dqdimacs file in the folder
while IFS= read -r -d '' file; do
    # Extract the file name without extension
    filename=$(basename "$file" .dqdimacs)
    
    echo "Running benchmark: $filename"
    
    # Record start time (in seconds)
    start_time=$(date +%s)
    
    # Run the main program using the `timeout` command
    timeout $TIMEOUT_LIMIT ./bin/main "$file" ./statistics/BDDtime.csv > "$LOG_DIR/${filename}.log" 2> "$LOG_DIR/${filename}.errlog"
    exit_code=$?
    
    # Check if the user pressed Ctrl+C (SIGINT)
    if [ $exit_code -eq 130 ]; then
        echo "Script interrupted by user."
        exit 130
    fi

    # Record end time and calculate elapsed time
    end_time=$(date +%s)
    elapsed_time=$((end_time - start_time))
    
    # Determine the result based on the exit code
    if [ $exit_code -eq 0 ]; then
        result="sat"
    elif [ $exit_code -eq 20 ]; then
        result="unsat"
    elif [ $exit_code -eq 124 ]; then
        result="timeout"
    elif [ $exit_code -eq 137 ]; then
        # Exit code 137 means the process was SIGKILLed, highly indicating an OOM event
        result="OOM"
    else
        result="error"
    fi
    
    # Save the time taken and result to the CSV
    echo "$filename,$elapsed_time,$result" >> "$STATS_FILE"
    
done < <(find "$TARGET_DIR" -type f -name "*.dqdimacs" -print0 | sort -z)

echo "Benchmarking completed successfully!"