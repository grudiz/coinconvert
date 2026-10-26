#!/bin/sh
# Install the committed HEAD of this repository with Homebrew.
set -eu

cd "$(dirname "$0")/.."
ROOT=$(pwd)

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "coinconvert: initialize the git repository before installing with Homebrew." >&2
    exit 1
fi

branch=$(git rev-parse --abbrev-ref HEAD)
if [ "$branch" != "main" ]; then
    echo "coinconvert: Homebrew installs the main branch (current branch is $branch)." >&2
    exit 1
fi

if [ -n "$(git status --porcelain)" ]; then
    echo "coinconvert: commit or stash changes first. Homebrew builds the committed main branch." >&2
    exit 1
fi

if brew tap 2>/dev/null | grep -qx "coinconvert/coinconvert"; then
    brew untap coinconvert/coinconvert
fi

brew tap coinconvert/coinconvert "$ROOT"

export HOMEBREW_NO_AUTO_UPDATE=1
export HOMEBREW_NO_INSTALL_FROM_API=1
export HOMEBREW_NO_ENV_HINTS=1
export HOMEBREW_NO_ASK=1

formula="coinconvert/coinconvert/coinconvert"
if brew list --formula coinconvert >/dev/null 2>&1; then
    brew reinstall --formula --yes "$formula"
else
    brew install --formula --yes "$formula"
fi

echo "Installed: $(command -v coinconvert)"
