#!/bin/sh

extfile="$1"
LD_LIBRARY_PATH="$HOME/.local/lib:/usr/local/lib64:/usr/lib/x86_64-linux-gnu"
LD_EXCLUDE_LIST="linux-vdso\.so.*
libc.so\.*
ld-linux-.*\.so.*
libglib-.*\.so.*
libpthread\.so.*
libm\.so.*
libstdc++\.so.*
libgcc_s\.so.*
libpcre\.so.*"

# Check dependencies
cmdlist="awk grep ldd patchelf"
for cmd in $cmlist
do
  if ! which $cmd > /dev/null
  then
    echo "Could not find ${cmd}. Is it installed?" > /dev/stderr
    exit 1
  fi
done

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
  ldd $extfile | while read ldd_line
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
  while true
  do
    ldd_output=$(ldd ${extfile})
    if echo "$ldd_output" | grep '=> not found' > /dev/null
    then
      next_missing=$(echo "$ldd_output"  | grep '=> not found' | head -n1 | awk '{print $1}')
      search_make_local_copy "$next_missing"
    else
      break
    fi
  done
}

find_missing
make_local_copy_and_set_rpath

