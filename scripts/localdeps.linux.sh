#!/bin/sh
#
# creates local copies of all dependencies (dynamic libraries)
# and sets RUNPATH to $ORIGIN on each so they will find
# each other.
#
# usage: $0 <binary>
#
# Author: Roman Haefeli <reduzent@gmail.com>

case $1 in
  "")
    echo "Usage: /bin/sh ${0} <binary>"
    exit
    ;;
  *)
    binary_file=$1
   ;;
esac

# List of libraries that we do not include into our packaging
# becaue we think they will be installed on any system
LD_EXCLUDE_LIST="linux-gate\.so.*
linux-vdso\.so.*
libarmmem.*\.so.*
libc.so\.*
ld-linux.*\.so.*
libdl\.so.*
libglib-.*\.so.*
libgomp\.so.*
libgthread.*\.so.*
libm\.so.*
libpthread.*\.so.*
libstdc++\.so.*
libgcc_s\.so.*
libpcre\.so.*"

# Check dependencies
cmdlist="awk grep ldd patchelf uname"
for cmd in $cmdlist
do
  if ! which $cmd > /dev/null
  then
    echo "Could not find ${cmd}. Is it installed?" > /dev/stderr
    exit 1
  fi
done

# Set LD_LIBRARY_PATH depending on arch
ARCH=$(uname -m)
case $ARCH in
  "x86_64")
    LD_LIBRARY_PATH="$HOME/.local/lib:/usr/local/lib64:/usr/lib/x86_64-linux-gnu"
    ;;
  "i686")
    LD_LIBRARY_PATH="$HOME/.local/lib:/usr/local/lib:/usr/lib/i386-linux-gnu"
    ;;
  "armv7l")
    LD_LIBRARY_PATH="$HOME/.local/lib:/usr/local/lib:/usr/lib/arm-linux-gnueabihf"
    ;;
  *)
    echo "Arch '$ARCH' not (yet) supported. Please file a bug report"
    exit 1
esac

# Check if we can read from given file
if ! ldd $binary_file > /dev/null 2>&1
then
  echo "Can't read '${binary_file}'. Is it a binary file?" > /dev/stderr
  exit 1
fi

library_in_exclude_list() {
# arg1: library name
# returns 0 if arg1 is found in exclude list, otherwise 1
  libexname="$1"
  skip=1
  set -f
  for expat in $(echo "$LD_EXCLUDE_LIST")
  do
    if echo "$(basename $libexname)" | grep "${expat}" > /dev/null
    then
      skip=0
      break
    fi
  done
  set +f
  return $skip
}

search_make_local_copy() {
  # look for given library in all library paths
  # and make a local copy of it
  # arg1: name of the library to make a local copy of
  found=false
  IFSold=$IFS
  IFS=:
  for path in ${LD_LIBRARY_PATH}
  do
    [ -f "${path}/$1" ] && cp "${path}/$1" . && found=true && break
  done
  IFS=$IFSold
  if ! $found
  then
    echo "$1 not found" > /dev/stderr
    exit 1
  fi
}

make_local_copy_and_set_rpath() {
  # make a local copy of all linked libraries of given binary
  # and set RUNPATH to $ORIGIN (exclude "standard" libraries)
  # arg1: binary to check
  ldd $1 | while read ldd_line
  do
    libname=$(echo "$ldd_line" | awk '{ print $1 }')
    libpath=$(echo "$ldd_line" | awk '{ print $3 }')
    if ! [ -f "$(basename $libname)" ] && ! library_in_exclude_list "$libname"
    then
      if [ "$libpath" != "" ]
      then
        cp "$libpath" .
      elif echo "$libname" | grep '/' > /dev/null
      then
        cp "$libname" .
      else
        echo "Warning: could not make copy of '$libname'. Not found"
      fi
    fi
    if ! library_in_exclude_list "$libname"
    then
      patchelf --set-rpath \$ORIGIN "$(basename $libname)"
    fi
  done
}

find_missing() {
  # find libraries that are shown as 'not found' in ldd and
  # create a local copy of them.
  # arg1: binary file to check for missing links
  while true
  do
    ldd_output=$(ldd ${1})
    if echo "$ldd_output" | grep '=> not found' > /dev/null
    then
      next_missing=$(echo "$ldd_output"  | grep '=> not found' | head -n1 | awk '{print $1}')
      search_make_local_copy "$next_missing"
    else
      break
    fi
  done
}

find_missing $binary_file
make_local_copy_and_set_rpath $binary_file

# clean after ourselves
rm $0
