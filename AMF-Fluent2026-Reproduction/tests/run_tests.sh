#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .build
cc -std=c99 -Wall -Wextra -Wno-unused-parameter -DALLOW_DEMO_MATERIAL -Itests/stub -Iudf tests/test_material_properties.c -lm -o .build/test_material_properties
.build/test_material_properties
python scripts/preflight.py
