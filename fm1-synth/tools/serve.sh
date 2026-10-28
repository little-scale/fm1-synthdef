#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
make all
exec ../.venv/bin/python tools/server.py "$@"
