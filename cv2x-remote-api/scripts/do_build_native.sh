#!/bin/bash

# Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause-Clear
#

#This script build cv2x-remote-api client side proxy libraries, and acme if telux source path is
#specified
#The build machine should have libboost-all-dev, libasio-dev, libtinyxml2-dev package installed
#usage: do_build_native telux_header_path

#--------------------------------------- Function--------------------------------------------------
#download source from github
#$1: repo link
#$2: revision(tag or commit #)
#$3: project name
function download_source() {
    git clone $1
    retVal=$?
    if [ $retVal -ne 0 ]; then
        echo "git clone failed"
        exit $retVal
    fi

    cd $WORKSPACE/$3
    git checkout $2
}
#check if cmake version is good return 0 if installed version is higher then target version
#$1: target version
check_cmake_version() {
    NATIVE_VERSION=$(cmake --version | head -1 | cut -f3 -d" ")
    mapfile -t sorted < <(printf "%s\n" "$ver" "$cmp" | sort -V)
    if [[ ${sorted[0]} == "$1" ]]; then
        return 0
    else
        return 1
    fi
}

#apply patch to downloaded sources
#$1: source path
#$2: patch file

#---------------------------- End of Functions ---------------------------------------------------
#function apply_patch() {
#}
if [[ $# -lt 1 ]]; then
    echo "usage: do_build_native: telux_header_path [telux_src_path]"
    echo "       telux_header_path: Full path to TelSDK public header(mandatory)"
    echo "       telux_src_path: full path to telux source code, if specified, will build cv2x radio
    C-CAPI and acme"
    exit 1;
fi
TELSDK_HEADER_PATH=$1

WORKSPACE="$PWD/build"
INSTALL_PREFIX=$WORKSPACE/install
COMMONAPI_PATH=capicxx-core-runtime
COMMONAPI_SOMEIP_PATH=capicxx-someip-runtime
FASTDDS_PATH=Fast-DDS
FASTCDR_PATH=Fast-CDR
ASIO_PATH=asio
FOONATHAN_PATH=memory
TINYXML2_PATH=tinyxml2
VSOMEIP_PATH=vsomeip
CV2X_REMOTE_GEN_PATH=cv2x-remote-gen
CV2X_REMOTE_API_PATH=cv2x-remote-api

if [ -d $WORKSPACE ]; then
    echo "reuse WORKSPACE: $WORKSPACE"
else
    mkdir -p $WORKSPACE
    echo "WORKSPACE: $WORKSPACE"
fi

#build capicxx-core-runtime if it doesn't exsit
if [ ! -f $INSTALL_PREFIX/lib/libCommonAPI.so.3.2.0 ]; then
  cd $WORKSPACE

  if [ -d $COMMONAPI_PATH ]; then
    echo "Skipped downloading source for capicxx-core-runtime"
    cd $COMMONAPI_PATH
  else
    echo "Downloading source for capicxx-core-runtime"
    download_source "https://github.com/COVESA/capicxx-core-runtime.git" \
        "3.2.0" \
        "capicxx-core-runtime"
    if [ $? -ne 0]; then
        exit
    fi
  fi
  #now build it
  cd $WORKSPACE/$COMMONAPI_PATH
  if [ ! -d build ]; then
    mkdir build
  fi
  cd build
  cmake ../ -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX
  make
  make install
fi

if [ $? -ne 0 ]; then
    exit
fi

#Build vsomeip
if [ ! -f $INSTALL_PREFIX/lib/libvsomeip3.so.3.4.10 ]; then
    cd $WORKSPACE
    if [ -d $VSOMEIP_PATH ]; then
        echo "Skipped downloading vsomeip"
        cd $VSOMEIP_PATH
    else
        echo "Downloading source for vsomeip"
        download_source "https://github.com/COVESA/vsomeip.git//" \
            "64f8d4e872845f804a60f13e457e4df62cfbab61" \
            "vsomeip"
      if [ $? -ne 0 ]; then
        exit
      fi
    fi
    #apply cmake patch
    git apply $WORKSPACE/../vsomeip_cmake.patch

    #build it
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    cmake ../  -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX
    make
    make install
fi
#build capicxx-someip-runtime if it doesn't exsit in the install directory
if [ ! -f $INSTALL_PREFIX/lib/libCommonAPI-SomeIP.so.3.2.0 ]; then
  cd $WORKSPACE
  if [ -d $COMMONAPI_SOMEIP_PATH ]; then
    echo "Skipped downloading capicxx-someip-runtime"
    cd $COMMONAPI_SOMEIP_PATH
  else
    echo "DOwnloading source for capicxx-someip-runtime"
    download_source "https://github.com/COVESA/capicxx-someip-runtime.git" \
        "3.2.0" \
        "capicxx-someip-runtime"
    if [ $? -ne 0 ]; then
        exit
    fi
  fi
  #now build it
  cd $WORKSPACE/$COMMONAPI_SOMEIP_PATH
  if [ ! -d build ]; then
    mkdir build
  fi
  cd build
  cmake ../ -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX -DUSE_INSTALLED_COMMONAPI=ON
  make
  make install
fi

#build Fast-CDR if it doesn't exsit in the install directory
if [ ! -f $INSTALL_PREFIX/lib/libfastcdr.so.2.1.0 ]; then
    cd $WORKSPACE
    if [ -d $FASTCDR_PATH ]; then
        echo "Skipped downloading Fast-CDR"
        cd $FASTCDR_PATH
    else
        echo "Downloading source for Fast-CDR"
        download_source "https://github.com/eProsima/Fast-CDR.git" \
            "aa637f9c0eae77ec41bb9c91b57b10836faba9be" \
            "Fast-CDR"
      if [ $? -ne 0 ]; then
        exit
      fi
    fi
    #apply cmake patch
    git apply $WORKSPACE/../fastcdr_cmake_version.patch

    #build it
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    cmake ../  -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
        -DBUILD_TESTING=OFF \
        -DCHECK_DOCUMENTATION=OFF \
        -DBUILD_DOCUMENTATION=OFF \
        -DGENERATE_PDF_DOC=OFF
    make
    make install
fi

#build foonathan_memory_vendor
if [ ! -d $INSTALL_PREFIX/$FOONATHAN_PATH ]; then
    cd $WORKSPACE
    if [ -d $FOONATHAN_PATH ]; then
        echo "Skipped downloading foonathan_memory_vendor"
        cd $FOONATHAN_PATH
    else
        echo "Downloading  source"
        download_source "https://github.com/foonathan/memory.git" \
            "0f0775770fd1c506fa9c5ad566bd6ba59659db66" \
            "$FOONATHAN_PATH"
        if [ $? -ne 0 ]; then
          exit
        fi
        #apply cmake patch
        git apply $WORKSPACE/../foonathan_memory_vendor_cmake.patch
    fi
    #build it
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    echo "Running cmake for ${WORKSPACE}/${FOONATHAN_PATH} "
    cmake ../  -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
        -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF \
        -DFOONATHAN_MEMORY_BUILD_TESTS=OFF \
        -DFOONATHAN_MEMORY_BUILD_TOOLS=OFF \
        -DBUILD_SHARED_LIBS=ON \
        -DBUILD_MEMORY_TOOLS=OFF
    echo "running make for ${WORKSPACE}/${FOONATHAN_PATH}"
    make
    if [ $? -ne 0 ]; then
        echo "make failed slfkjslfj"
        #first time build could fail if the build machine cmake version is older.
        #apply patch
        echo "applying patch"
        cd $WORKSPACE/$FOONATHAN_PATH/
        git apply $WORKSPACE/../foo_mem-ext_cmake.patch
        cd $WORKSPACE/$FOONATHAN_PATH/build
        #build again
        make
    fi
    make install
fi

#build Fast-DDS
if [ ! -f $INSTALL_PREFIX/lib/libfastrtps.so.2.13.0 ]; then
    cd $WORKSPACE
    if [ -d $FASTDDS_PATH ]; then
        echo "Skipped downloading Fast-DDS"
        cd $FASTDDS_PATH
    else
        echo "Downloading source for Fast-DDS"
        download_source "https://github.com/eProsima/Fast-DDS.git" \
            "00953731a1e2724d748e4293f4aa295537d149b7" \
            "Fast-DDS"
      if [ $? -ne 0 ]; then
        exit
      fi
    fi
    #apply cmake patch
    git apply $WORKSPACE/../fastdds_cmake_version.patch

    #build it
    if [ ! -d build ]; then
        mkdir build
    fi
    cd build
    cmake ../  -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
        -DCOMPILE_TOOLS=OFF \
        -DEPROSIMA_BUILD=OFF \
        -DCHECK_DOCUMENTATION=OFF \
        -DBUILD_DOCUMENTATION=OFF \
        -DSQLITE3_SUPPORT=OFF \
        -DFASTDDS_STATISTICS=OFF \
        -DCOMPILE_EXAMPLES=OFF \
        -DINSTALL_EXAMPLES=OFF \
        -DINSTALL_TOOLS=OFF \
        -DINTERNAL_DEBUG=OFF \
        -DIS_THIRDPARTY_BOOST_OK=OFF
    make
    make install
fi

#build cv2x-remote-gen (library from IDL generated files)
if [ ! -f $INSTALL_PREFIX/lib/libCv2xRemoteAPI-someip.so ]; then
    cd $WORKSPACE
    if [ ! -d $CV2X_REMOTE_GEN_PATH ]; then
        mkdir $CV2X_REMOTE_GEN_PATH
    fi
    cd $WORKSPACE/$CV2X_REMOTE_GEN_PATH
    cmake $WORKSPACE/../../../../cv2x-oss-other/cv2x-remote-gen/ -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
        -DTARGET=CLIENT \
        -DUSE_CONSOLE=ON \
        -DTELUX_HEADER_PATH=$1
    make
    make install
fi

#build cv2x-remote-api client libraries
if [ ! -f $INSTALL_PREFIX/lib/libtelux_cv2x_remote.so ]; then
    cd $WORKSPACE
    if [ ! -d $CV2X_REMOTE_API_PATH ]; then
        mkdir $CV2X_REMOTE_API_PATH
    fi
    if [[ ! -z $2 ]]; then
        #telsdk source path is availabe, will build cv2x radio C API, this needs nanopb
        #check if nanopb is in install path
        if [ ! -f $INSTALL_PREFIX/lib/libprotobuf-nanopb.a ]; then
            if [ -d nanopb ]; then
                echo "skipped downloading nanopb"
            else
                echo "Downloading nanopb"
                download_source "https://github.com/nanopb/nanopb.git" \
                     "nanopb-0.3.8" \
                     "nanopb"
                if [ $? -ne 0 ]; then
                    exit
                fi
                if [ ! -d build ]; then
                    mkdir build
                fi
                cd build
                cmake ../ -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
                  -Dnanopb_BUILD_GENERATOR=OFF \
                  -Dnanopb_MSVC_STATIC_RUNTIME=OFF
                make
                make install
            fi
        fi
    fi
    cd $WORKSPACE/$CV2X_REMOTE_API_PATH
    cmake $WORKSPACE/../../ -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX \
        -DTARGET=CLIENT \
        -DUSE_CONSOLE=ON \
        -DTELUX_HEADER_PATH=$1 \
        -DTELUX_SRC_PATH=$2 \
        -DSYS_INCLUDE=$INSTALL_PREFIX/include \
        -DCMAKE_LIBRARY_PATH=$INSTALL_PREFIX \
        -DBUILD_ACME=ON
    make
    make install
fi
