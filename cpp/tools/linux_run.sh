#! /bin/bash -e

# Get current script path
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Get PaddleOCR-json path
EXE_LOCATION="$SCRIPT_DIR/../build/bin/PaddleOCR-json"

# All PaddleOCR runtime libraries are copied to the same path as PaddleOCR-json
LIBS="$(dirname $EXE_LOCATION)"

# Run PaddleOCR-json
LD_LIBRARY_PATH="$LIBS" "$EXE_LOCATION" \
    "$@"
