#!/bin/sh
# Install coinconvert with Homebrew. This does not need a clone of the source.
set -eu

if ! command -v brew >/dev/null 2>&1; then
    echo "coinconvert: Homebrew is required. Install it from https://brew.sh and run this script again." >&2
    exit 1
fi

if brew list --formula coinconvert >/dev/null 2>&1; then
    echo "coinconvert is already installed: $(command -v coinconvert)"
    exit 0
fi

brew install grudiz/tap/coinconvert
echo "Installed: $(command -v coinconvert)"
