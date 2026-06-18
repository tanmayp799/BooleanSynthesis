#!/bin/bash
if [ -z "$1" ]; then exit 1; fi

# 1. Setup absolute paths
BENCHMARK_PATH=$(readlink -f "$1")
FILENAME=$(basename "$BENCHMARK_PATH")
PROJECT_ROOT=$(pwd)

# 2. Match your exact experiment folder structure
mkdir -p "$PROJECT_ROOT/experiment/stats/app2"
mkdir -p "$PROJECT_ROOT/experiment/basis_a/app2"
mkdir -p "$PROJECT_ROOT/experiment/basis_b/app2"
mkdir -p "$PROJECT_ROOT/experiment/stdout"
mkdir -p "$PROJECT_ROOT/sandboxes"

# 3. Create isolated workspace
WORKSPACE="$PROJECT_ROOT/sandboxes/$FILENAME"
rm -rf "$WORKSPACE"
mkdir -p "$WORKSPACE"

# 4. Symlink the standard heavy stuff & your experiment folder
ln -s "$PROJECT_ROOT/bin" "$WORKSPACE/bin"
ln -s "$PROJECT_ROOT/experiment" "$WORKSPACE/experiment"

mkdir -p "$WORKSPACE/dependencies"
ln -s "$PROJECT_ROOT/dependencies/abc" "$WORKSPACE/dependencies/abc"
ln -s "$PROJECT_ROOT/dependencies/cadical" "$WORKSPACE/dependencies/cadical"

# 5. The Smart Clone: Copy Manthan scripts, SYMLINK the heavy virtual environment
# rsync -a --exclude='manthan-venv' "$PROJECT_ROOT/dependencies/manthan/" "$WORKSPACE/dependencies/manthan/"
# ln -s "$PROJECT_ROOT/dependencies/manthan/manthan-venv" "$WORKSPACE/dependencies/manthan/manthan-venv"

# 6. Copy the lightweight test scripts
# cp -r "$PROJECT_ROOT/manthan_test" "$WORKSPACE/manthan_test"
# mkdir -p "$WORKSPACE/assignments_NoUnit"

# THE FIX: Symlink benchmark_tests directly into the specific sandbox workspace
ln -s "$PROJECT_ROOT/benchmark_tests" "$WORKSPACE/benchmark_tests"

# 7. Jump in and run
cd "$WORKSPACE"
./bin/main "$BENCHMARK_PATH" > "$PROJECT_ROOT/experiment/stdout/${FILENAME%.*}.log" 2>&1

# 8. Clean up
cd "$PROJECT_ROOT"
rm -rf "$WORKSPACE"
