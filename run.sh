#! /usr/bin/env bash

# Run the application
echo "Running the application... Mode: ($1)"
cmake --build build --config $1
"./build/$1/Application/App"
