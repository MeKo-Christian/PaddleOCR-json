#! /bin/bash -e

# Get current script path
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# Get PaddleOCR-json path
EXE_LOCATION="$SCRIPT_DIR/bin/"
LIB_LOCATION="$SCRIPT_DIR/lib/"

# Write PaddleOCR-json and its dependencies paths to ~/.bashrc
eval echo 'export PATH=$PATH:$EXE_LOCATION' >> ~/.bashrc
eval echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$LIB_LOCATION' >> ~/.bashrc

echo ""
echo "You need to restart current shell session, or run following command in order for the install to take effect:"
echo "You need to restart current shell session, or run the following command to complete the installation:"
echo ""
echo "source ~/.bashrc"
echo ""
