#! /bin/bash -e

# Get current script path and go to "cpp" folder
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
cd $SCRIPT_DIR/..

# Some CMake variables
MODE=Release
PADDLE_LIB="$(pwd)/$(ls -d .source/*paddle_inference*/ | head -n1)"

echo "MODE: $MODE"
echo "PADDLE_LIB: $PADDLE_LIB"

# Build & compile project
mkdir -p build

# If build folder is not a CMake project, build project folder (first run)
if [ ! -d "build/CMakeFiles/" ]; then
    cmake -S . -B build/ -DPADDLE_LIB="$PADDLE_LIB" -DCMAKE_BUILD_TYPE="$MODE"
fi

# Compile
cmake --build build/ --config="$MODE"
