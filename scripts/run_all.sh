#!/bin/bash

cd "$(dirname "$0")"

for script in *.py; do
    if [ "$script" != "benchmark.py" ]; then
        echo "Running: $script"
        python3 "$script"
        echo "Done: $script"
        echo "---"
    fi
done
