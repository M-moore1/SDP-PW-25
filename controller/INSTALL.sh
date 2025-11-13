#!/bin/bash

# Installation script for WebSocket Controller
# This script sets up the project and optionally starts the test server

set -e  # Exit on error

echo "🚀 WebSocket Controller - Installation Script"
echo "=============================================="
echo ""

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    echo "❌ Error: Node.js is not installed"
    echo "Please install Node.js from https://nodejs.org/"
    exit 1
fi

echo "✅ Node.js version: $(node --version)"
echo "✅ npm version: $(npm --version)"
echo ""

# Navigate to script directory
cd "$(dirname "$0")"

# Install dependencies
echo "📦 Installing dependencies..."
npm install

if [ $? -eq 0 ]; then
    echo "✅ Dependencies installed successfully"
else
    echo "❌ Failed to install dependencies"
    exit 1
fi

echo ""

# Create .env file if it doesn't exist
if [ ! -f .env ]; then
    echo "📝 Creating .env file..."
    echo "VITE_WS_URL=ws://localhost:8080" > .env
    echo "✅ .env file created with default settings"
else
    echo "ℹ️  .env file already exists"
fi

echo ""
echo "=============================================="
echo "✨ Installation complete!"
echo ""
echo "Available commands:"
echo "  npm run dev     - Start development server"
echo "  npm run build   - Build for production"
echo "  npm run preview - Preview production build"
echo ""
echo "To test with the included WebSocket server:"
echo "  1. Install server dependencies: npm install ws"
echo "  2. Run: node test-server.js"
echo "  3. In another terminal: npm run dev"
echo ""
echo "Happy coding! 🎮"

