#!/bin/bash
set -e

# Generate car model if not present
if [ ! -f assets/car_body.obj ]; then
    echo "Generating car model..."
    python3 tools/generate_car.py
fi

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/city_drift_game
