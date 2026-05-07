#!/bin/bash

# Configuration
TARGET_DIR="./small_tests/"
LOG_DIR="./logs/bdd_tests"
STATS_DIR="./statistics"
FINAL_STATS_FILE="$STATS_DIR/BDD_timeout.csv"
TIMEOUT_LIMIT=3600
JOBS=2 # Use 32 for cn01

# Preparation
mkdir -p "$LOG_DIR"
mkdir -p "$STATS_DIR/tmp_per_job"
echo "benchmark,time(s),result" > "$FINAL_STATS_FILE"

# The Function
run_benchmark() {
    file="$1"
    TIMEOUT_LIMIT="$2"
    LOG_DIR="$3"
    STATS_DIR="$4"
    
    filename=$(basename "$file" .dqdimacs)
    job_bdd_csv="$STATS_DIR/tmp_per_job/${filename}_BDDtime.csv"
    
    start_ts=$(date +%s.%N)
    
    # Execution
    nice -n 19 timeout "$TIMEOUT_LIMIT" ./bin/main "$file" "$job_bdd_csv" > "$LOG_DIR/${filename}.log" 2> "$LOG_DIR/${filename}.errlog"
    exit_code=$?
    
    end_ts=$(date +%s.%N)
    elapsed_time=$(echo "$end_ts - start_ts" | bc)
    
    case $exit_code in
        0)   result="sat" ;;
        20)  result="unsat" ;;
        124) result="timeout" ;;
        137) result="OOM" ;;
        *)   result="error" ;;
    esac
    
    echo "$filename,$elapsed_time,$result"
}

# Export the function so sub-shells can see it
export -f run_benchmark

echo "Starting Parallel Run..."

# --- THE CLEAN CALL ---
# 1. We remove --halt to avoid the parsing error.
# 2. We use --env to explicitly pass the function and variables.
find "$TARGET_DIR" -type f -name "*.dqdimacs" | sort | \
parallel --env run_benchmark -j "$JOBS" \
run_benchmark {} "$TIMEOUT_LIMIT" "$LOG_DIR" "$STATS_DIR" >> "$FINAL_STATS_FILE"

echo "Done. Results saved to $FINAL_STATS_FILE"