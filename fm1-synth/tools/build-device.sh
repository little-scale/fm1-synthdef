#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/../.."
source setup/env.sh
export X0X_FONTSET=terminus
python3 setup/verify-sdk.py
python3 fm1-base/tools/build.py fm1-synth/firmware --no-ble
