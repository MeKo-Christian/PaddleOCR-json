#! /bin/bash -e

# Get current script path
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Get PaddleOCR-json path
EXE_LOCATION="$SCRIPT_DIR/bin/PaddleOCR-json"
LIB_LOCATION="$SCRIPT_DIR/lib/"

# Run PaddleOCR-json
LD_LIBRARY_PATH="$LIB_LOCATION" "$EXE_LOCATION" -models_path="$SCRIPT_DIR/models" "$@"
