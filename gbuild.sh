#   This file is part of LIMA.
#
#   LIMA is free software: you can redistribute it and/or modify
#   it under the terms of the GNU Affero General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   LIMA is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU Affero General Public License for more details.
#
#   You should have received a copy of the GNU Affero General Public License
#   along with LIMA.  If not, see <http://www.gnu.org/licenses/>
#!/bin/bash

#Fail if anything goes wrong 
# set -o errexit
# set -o nounset
# set -o xtrace

usage() 
{ 
cat << EOF 1>&2; exit 1; 
Synopsis: $0 [OPTIONS]

Options default values are in parentheses.

  -a asan           <(OFF)|ON> compile with adress sanitizer
  -d debug-messages <(OFF)|ON> compile with debug messages on in release mode
  -m mode           <(Debug)|Release|RelWithDebInfo> compile mode
  -n arch           <(generic)|native> target architecture mode
  -r resources      <precompiled|(build)> build the linguistic resources or use the
                    precompiled ones
  -v version        <(val)|rev> version number is set either to the value set by  
                    config files or to the short git sha1
  -G Generator      <(Ninja)|Unix|MSYS|NMake|VS> which cmake generator to use.
  -T                Use TensorFlow
  -P tfsrcpath      <> Path to TensorFlow sources
EOF
exit 1
}

[ -z "$LIMA_BUILD_DIR" ] && echo "Need to set LIMA_BUILD_DIR" && exit 1;
[ -z "$LIMA_DIST" ] && echo "Need to set LIMA_DIST" && exit 1;

arch="generic"
WITH_DEBUG_MESSAGES="OFF"
mode="Debug"
version="val"
resources="build"
CMAKE_GENERATOR="Ninja"
WITH_ASAN="OFF"
WITH_ARCH="OFF"
SHORTEN_POR_CORPUS_FOR_SVMLEARN="OFF"
USE_TF=false
TF_SOURCES_PATH=""

while getopts ":d:m:n:r:v:G:a:P:T" o; do
    case "${o}" in
        a)
            WITH_ASAN=${OPTARG}
            [[ "$WITH_ASAN" == "ON" || "$WITH_ASAN" == "OFF" ]] || usage
            ;;
        d)
            WITH_DEBUG_MESSAGES=${OPTARG}
            [[ "x$WITH_DEBUG_MESSAGES" == "xON" || "x$WITH_DEBUG_MESSAGES" == "xOFF" ]] || usage
            ;;
        m)
            mode=${OPTARG}
            [[ "$mode" == "Debug" || "$mode" == "Release"  || "$mode" == "RelWithDebInfo" ]] || usage
            ;;
        n)
            arch=${OPTARG}
            [[ "x$arch" == "xnative" || "x$arch" == "xgeneric" ]] || usage
            ;;
        G)
            CMAKE_GENERATOR=${OPTARG}
            echo "CMAKE_GENERATOR=$CMAKE_GENERATOR"
            [[     "$CMAKE_GENERATOR" == "Unix"  ||
                   "$CMAKE_GENERATOR" == "Ninja" ||
                   "$CMAKE_GENERATOR" == "MSYS"  ||
                   "$CMAKE_GENERATOR" == "NMake" ||
                   "$CMAKE_GENERATOR" == "VS"
            ]] || usage
            ;;
        r)
            resources=${OPTARG}
            [[ "$resources" == "precompiled" || "$resources" == "build" ]] || usage
            ;;
        v)
            version=$OPTARG
            [[ "$version" == "val" ||  "$version" == "rev" ]] || usage
            ;;
        T)
            USE_TF=true
            ;;
        P)
            TF_SOURCES_PATH=$OPTARG
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if type git && git rev-parse --git-dir; then
    current_branch=$(git rev-parse --abbrev-ref HEAD)
    current_revision=$(git rev-parse --short HEAD)
    current_timestamp=$(git show -s --format=%cI HEAD | sed -e 's/[^0-9]//g')
else
    # use default values
    current_branch="default"
    current_revision="default"
    current_timestamp=1
fi
current_project=`basename $PWD`
current_project_name="`head -n1 CMakeLists.txt`"
build_prefix=$LIMA_BUILD_DIR/$current_branch
source_dir=$PWD

if [[ $version = "rev" ]]; then
  release="$current_timestamp-$current_revision"
else
  release="0"
fi

j="1"
if [[ $CMAKE_GENERATOR == "VS" ]]; then
  j=`WMIC CPU Get NumberOfCores | head -n 2 | tail -n 1 | sed -n "s/\s//gp"`
elif [[ $CMAKE_GENERATOR == "Unix" ]]; then
  j=`grep -c ^processor /proc/cpuinfo`
fi
echo "Parallel build on $j processors"

# export VERBOSE=1
if [[ $mode == "Release" ]]; then
  cmake_mode="Release"
  SHORTEN_POR_CORPUS_FOR_SVMLEARN="OFF"
elif [[ $mode == "RelWithDebInfo" ]]; then
  cmake_mode="RelWithDebInfo"
  SHORTEN_POR_CORPUS_FOR_SVMLEARN="ON"
else
  cmake_mode="Debug"
  SHORTEN_POR_CORPUS_FOR_SVMLEARN="ON"
fi

if [[ $arch == "native" ]]; then
  WITH_ARCH="ON"
else
  WITH_ARCH="OFF"
fi

if [[ $CMAKE_GENERATOR == "Unix" ]]; then
  make_cmd="make -j$j"
  make_test="make test"
  make_install="make install"
  generator="Unix Makefiles"
elif [[ $CMAKE_GENERATOR == "Ninja" ]]; then
  make_cmd="ninja"
  make_test=""
  make_install="ninja install"
  generator="Ninja"
elif [[ $CMAKE_GENERATOR == "MSYS" ]]; then
  make_cmd="make -j$j"
  make_test="make test"
  make_install="make install"
  generator="MSYS Makefiles"
elif [[ $CMAKE_GENERATOR == "NMake" ]]; then
  make_cmd="nmake && exit 0"
  make_test="nmake test"
  make_install="nmake install"
  generator="NMake Makefiles"
elif [[ $CMAKE_GENERATOR == "VS" ]]; then
  make_cmd="""
  pwd &&
  cmake --build . --config $cmake_mode
  """
  make_test=""
  make_install=""
  generator="Visual Studio 14 2015 Win64"
else
  make_cmd="make -j$j"
fi

echo "version='$release'"

if [[ $CMAKE_GENERATOR == "VS" ]]; then
  #consider linking this current place to $LIMA_BUILD_DIR if different
  #this could be usefull to trick windows path length limitation
  #when building with VS
  mkdir -p $LIMA_BUILD_DIR
  pushd $LIMA_BUILD_DIR
else
  mkdir -p $build_prefix/$mode/$current_project
  pushd $build_prefix/$mode/$current_project
fi

if [ "$USE_TF" = false ] ; then
  TF_SOURCES_PATH=""
else
  if [ ${#TF_SOURCES_PATH} -le 0 ] ; then
    TF_SOURCES_PATH=/usr/include/tensorflow-for-lima/
  fi

  echo "Path to TensorFlow sources: $TF_SOURCES_PATH"
  echo "Copying TensorFlow binaries from /usr/lib/ to $LIMA_DIST/lib"

  mkdir -p $LIMA_DIST/lib
  if [ -f /usr/lib/libtensorflow-for-lima.so ]; then
    cp /usr/lib/libtensorflow-for-lima.so $LIMA_DIST/lib/
  elif [ -f /usr/lib/libtensorflow_cc.so ] && [ -f /usr/lib/libtensorflow_framework.so ]; then
    cp /usr/lib/libtensorflow_cc.so $LIMA_DIST/lib/
    cp /usr/lib/libtensorflow_framework.so $LIMA_DIST/lib/
  fi
fi

echo "Launching cmake from $PWD"
cmake  -G "$generator" -DWITH_DEBUG_MESSAGES=$WITH_DEBUG_MESSAGES -DWITH_ARCH=$WITH_ARCH -DWITH_ASAN=$WITH_ASAN -DSHORTEN_POR_CORPUS_FOR_SVMLEARN=$SHORTEN_POR_CORPUS_FOR_SVMLEARN -DCMAKE_BUILD_TYPE:STRING=$cmake_mode -DLIMA_RESOURCES:PATH="$resources" -DLIMA_VERSION_RELEASE:STRING="$release" -DCMAKE_INSTALL_PREFIX:PATH=$LIMA_DIST -DTF_SOURCES_PATH:PATH=$TF_SOURCES_PATH $source_dir

echo "Running command:"
echo "$make_cmd"
eval $make_cmd
result=$?

#exit $result

if [ "x$current_project_name" != "xproject(Lima)" ];
then
  eval $make_test && eval $make_install
  result=$?
fi

if [ $CMAKE_GENERATOR == "Unix" ] && [ "x$cmake_mode" == "xRelease" ] ;
then
  install -d $LIMA_DIST/share/apps/lima/packages
  if compgen -G "*/src/*-build/*.rpm" > /dev/null; then
    install */src/*-build/*.rpm $LIMA_DIST/share/apps/lima/packages
  fi
  if compgen -G "*/src/*-build/*.deb" > /dev/null; then
    install */src/*-build/*.deb $LIMA_DIST/share/apps/lima/packages
  fi
fi

popd

exit $result
