#!/bin/bash

# Start the production server in the background
node server/production-server.js &
SERVER_PID=$!

echo "Started production server (PID: $SERVER_PID)"

# Navigate to ECE/GS and build
cd ECE/GS || { echo "Failed to cd into ECE/GS"; kill $SERVER_PID; exit 1; }

echo "Running make clean..."
make clean

echo "Running make all..."
make all

echo "Build complete."