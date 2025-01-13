#!/bin/bash

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#

# This script take care of download commonapi_core_generator, commonapi_someip_generator and
# fast-dds-gen tools and generate c++ source code according the fidl, fdepl and idl protol files.
# usage: ./update_SOMEIP_interface.sh <FIDL_PROTO_FOLDER_PATH> <OPTIONAL_output_path>

echo $# arguments
if [ $# -eq 0 ]; then
    echo "empty argument, INVALID usage!"
    echo "./update_SOMEIP_interface.sh <FIDL_PROTO_FOLDER_PATH> <OPTIONAL_output_path>"
    exit
fi

FIDL_PROTO_PATH=$1
WORKSPACE="$PWD"
CPP_OUTPUT_PATH="$PWD"

if [[ ! -z $2 ]]; then
    CPP_OUTPUT_PATH=$2
fi

ARCH="$(uname -m)"
echo "${ARCH}"

if [ ! -d $WORKSPACE/tools ]; then
    echo "mkdir tools"
    mkdir $WORKSPACE/tools
fi

if [ ! -f $WORKSPACE/tools/core/commonapi-core-generator-linux-${ARCH} ]; then
    cd $WORKSPACE/tools
    if [ ! -f $WORKSPACE/tools/commonapi_core_generator.zip ]; then
        wget -nv https://github.com/COVESA/capicxx-core-tools/releases/download/3.2.0.1/commonapi_core_generator.zip
        if [ $? -ne 0 ]; then
            exit
        fi
    fi
    if [ ! -d $WORKSPACE/tools/core ]; then
        echo "mkdir tools/core"
        mkdir $WORKSPACE/tools/core
    fi
    unzip -q -o ./commonapi_core_generator.zip -d $WORKSPACE/tools/core
fi
cd $WORKSPACE
$WORKSPACE/tools/core/commonapi-core-generator-linux-${ARCH} -sk \
    -d $CPP_OUTPUT_PATH/core $FIDL_PROTO_PATH/Cv2x.fidl
$WORKSPACE/tools/core/commonapi-core-generator-linux-${ARCH} -sk \
    -d $CPP_OUTPUT_PATH/core $FIDL_PROTO_PATH/common-core.fdepl

#download commonapi_someip_generator and generate source code
if [ ! -f $WORKSPACE/tools/someip/commonapi-someip-generator-linux-${ARCH} ]; then
    cd $WORKSPACE/tools
    if [ ! -f $WORKSPACE/tools/commonapi_someip_generator.zip ]; then
        wget -nv https://github.com/COVESA/capicxx-someip-tools/releases/download/3.2.0.1/commonapi_someip_generator.zip
        if [ $? -ne 0 ]; then
            exit
        fi
    fi
    if [ ! -d $WORKSPACE/tools/someip ]; then
        echo "mkdir tools/someip"
        mkdir $WORKSPACE/tools/someip
    fi
    unzip -q -o commonapi_someip_generator.zip -d $WORKSPACE/tools/someip
fi
cd $WORKSPACE
$WORKSPACE/tools/someip/commonapi-someip-generator-linux-${ARCH} -d $CPP_OUTPUT_PATH/someip \
    $FIDL_PROTO_PATH/Cv2xRadioManager-SomeIp.fdepl
$WORKSPACE/tools/someip/commonapi-someip-generator-linux-${ARCH} -d $CPP_OUTPUT_PATH/someip \
    $FIDL_PROTO_PATH/Cv2xRadio-SomeIp.fdepl