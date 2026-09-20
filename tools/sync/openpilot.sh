#!/usr/bin/env bash
set -e

# Syncs openpilot's dependency list into the image's Python environment.
#
# openpilot owns the list of packages it needs; userspace/uv/pyproject.toml adds
# what the image needs on top and pins openpilot's submodules as Git sources, so
# their dependencies come along too. On device the code itself comes from
# /data/openpilot via PYTHONPATH, only the dependency metadata is used here.
#
# Run this whenever openpilot's pyproject.toml changes, then rebuild the system
# image.

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." >/dev/null && pwd)"

REPO="${OPENPILOT_REPO:-dorapilot/openpilot}"
REF="${1:-${OPENPILOT_REF:-liberation-day-7.2}}"

if ! command -v uv > /dev/null 2>&1; then
  echo "installing uv..."
  curl -LsSf https://astral.sh/uv/install.sh | sh
fi

cd "$DIR/userspace/uv"

echo "syncing dependencies from $REPO@$REF"
curl -fsSo openpilot/pyproject.toml.tmp "https://raw.githubusercontent.com/$REPO/$REF/pyproject.toml"

# openpilot's sources point at its own checkout, ours are in pyproject.toml
awk '/^\[tool\.uv\.sources\]/{exit} {buf = buf $0 "\n"} END{sub(/\n+$/, "\n", buf); printf "%s", buf}' \
  openpilot/pyproject.toml.tmp > openpilot/pyproject.toml
rm openpilot/pyproject.toml.tmp

uv lock --upgrade
