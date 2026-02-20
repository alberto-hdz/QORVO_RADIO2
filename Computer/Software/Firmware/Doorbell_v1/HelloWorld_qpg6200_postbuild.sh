#!/bin/sh

set -e

SCRIPT_DIR="$(dirname "$(realpath "$0")")"

# determine python interpreter path
if [ -f "`which python3`" ]; then
    PYTHON="`which python3`"
else
    if [ -f "`which python`" ]; then
        PYTHON="`which python`"
    else
        echo "No python interpreter found."
        exit 1
    fi
fi

# check python interpreter version
PYTHON_VERSION=`"${PYTHON}" --version | cut -d\  -f 2`
PYTHON_MAJOR=`echo ${PYTHON_VERSION} | cut -d. -f 1`
PYTHON_MINOR=`echo ${PYTHON_VERSION} | cut -d. -f 2`

if [ ! \( ${PYTHON_MAJOR} -eq 3 -o ${PYTHON_MINOR} -lt 6 \) ]; then
    echo "Python 3.x (at least 3.6) is required, you have ${PYTHON_VERSION}."
fi

# seed random source

RANDOM=`date +%s`$$

# set variables

OLD_CWD=`pwd`
TEMP_DIR=/tmp
UNIQUE_ID=${RANDOM}
PROJECT_PATH="$1"
TARGET_PATH="$2"
TARGET_BASEPATH="`echo ${TARGET_PATH} | sed -E 's/\.[^.]+$//g'`"
TARGET_BASENAME="`basename ${TARGET_BASEPATH}`"
TARGET_DIR="`dirname ${TARGET_BASEPATH}`"

trap 'cd ${OLD_CWD}' EXIT

# Build steps

cp "${TARGET_BASEPATH}.hex" "${TARGET_BASEPATH}_before_signing.hex_"

appuc-firmware-packer --appuc 1 --version 1 --input ${TARGET_BASEPATH}_before_signing.hex_ --sign "${SCRIPT_DIR}"/../../Tools/SecureBoot/developer_key_private.der --cert "${SCRIPT_DIR}"/../../Tools/SecureBoot/developer_certificate_signed.cert --output ${TARGET_BASEPATH}.hex

cp "${TARGET_BASEPATH}.hex" "${TARGET_BASEPATH}_before_hexmerge.hex_"

"$PYTHON" "${SCRIPT_DIR}"/../../Tools/Hex/hexmerge.py ${TARGET_BASEPATH}.hex ${TARGET_BASEPATH}_before_hexmerge.hex_ "${SCRIPT_DIR}"/../../Work/Bootloader_qpg6200/Bootloader_qpg6200.hex --ignore_start_execution_addr --overlap keep_last
