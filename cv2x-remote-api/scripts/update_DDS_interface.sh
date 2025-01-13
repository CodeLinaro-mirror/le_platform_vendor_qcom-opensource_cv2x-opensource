#!/bin/bash

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#

# This script take care of download Fast-DDS-Gen tool, and generate c++ source code according to
# fastDDS idl protol files.
# usage: ./update_DDS_interface.sh <IDL_PROTO_FOLDER_PATH>
# input parameter IDL_PROTO_FOLDER_PATH is not only the IDL proto path but also the output path

#--------------------------------------- Function--------------------------------------------------
#download source from website
#$1: repo link
#$2: revision(tag or commit #)
#$3: project name
function download_source() {
    git clone $1 $3
    retVal=$?
    if [ $retVal -ne 0 ]; then
        echo "git clone failed"
        exit $retVal
    fi

    cd $3
    git checkout $2
}

WORKSPACE="$PWD"

echo $# arguments
if [ $# -eq 0 ]; then
    echo "empty argument, INVALID usage!"
    echo "./update_DDS_interface.sh <IDL_PROTO_FOLDER_PATH>"
    echo "<IDL_PROTO_FOLDER_PATH> is not only the IDL proto path but also the output path"
    exit
fi

FDDS_PROTO_PATH=$1
FDDS_GEN_FOLDER="Fast-DDS-Gen"
FDDS_GEN_FULL_PATH=$WORKSPACE/tools/$FDDS_GEN_FOLDER

if [ ! -d $WORKSPACE/tools ]; then
    echo "mkdir tools folder"
    mkdir $WORKSPACE/tools
fi

if [[ ! -z $1 ]]; then
    #FAST-DDS-GEN proto source path is availabe
    if [ ! -d $FDDS_GEN_FULL_PATH ]; then
        cd $WORKSPACE/tools
        download_source "--recursive https://github.com/eProsima/Fast-DDS-Gen.git" \
            "5b699449462f1dcfe9401a9d0ee2090d3bc6e443" \
            "${FDDS_GEN_FOLDER}"
    fi
    if [ -d $FDDS_GEN_FULL_PATH ]; then
        if [ ! -d $FDDS_GEN_FULL_PATH/build ]; then
            cd $FDDS_GEN_FULL_PATH
            ./gradlew assemble
        fi
    fi

    if [ -f $FDDS_GEN_FULL_PATH/scripts/fastddsgen ]; then
        cd $WORKSPACE
        $FDDS_GEN_FULL_PATH/scripts/fastddsgen -replace -cs  $FDDS_PROTO_PATH/Cv2xData.idl \
            -d $FDDS_PROTO_PATH
    else
        echo "ERROR: fastddsgen binary not exists"
    fi
fi