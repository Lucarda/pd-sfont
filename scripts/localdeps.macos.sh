#!/bin/bash

## puts dependencies besides the binary
# LATER: put dependencies into a separate folder

## usage: $0 <binary> [<binary2>...]


ARGS=()
recursion=false

while [[ $# -gt 0 ]]; do
  case $1 in
    -r|--recursive)
      recursion=true
      shift
      ;;
    -*|--*)
      echo "Unknown option $1"
      exit 1
      ;;
    *)
      ARGS+=("$1") # save positional arg
      shift # past argument
      ;;
  esac
done


error() {
   echo "$@" 1>&2
}
basename () {
  local x=${1##*/}
  if [ "x$x" = "x" ]; then
     echo $1
  else
     echo $x
  fi
}
dirname () {
  local x=${1%/*}
  if [ "x$x" = "x" ]; then
     echo .
  else
     echo $x
  fi
}

arch=$(uname -m)
if [ "$arch" = "x86_64" ]; then
  arch="amd64"
fi

if [ "x${otool}" = "x" ]; then
 otool="otool -L"
fi
if [ "x${otool}" = "x" ]; then
  error "no 'otool' binary found"
  exit 0
fi

list_deps() {
  $otool "$1" \
        | grep -v ":$" \
	| grep compatibility \
	| awk '{print $1}' \
	| egrep '^/' \
	| egrep -v '^/usr/lib/' \
	| egrep -v '^/System/Library/Frameworks'
}

install_deps () {
error "DEP: ${INSTALLDEPS_INDENT}$1"
outdir=$2
if [ "x${outdir}" = "x" ]; then
  outdir="$(dirname "$1")"
fi
if [ ! -d "${outdir}" ]; then
  outdir=.
fi

if ! $recursion; then
  outdir="${outdir}/${arch}"
  mkdir -p "${outdir}"
fi

list_deps "$1" | while read dep; do
  infile=$(basename "$1")
  depfile=$(basename "${dep}")
  if $recursion; then
    loaderpath="@loader_path/${depfile}"
  else
    loaderpath="@loader_path/${arch}/${depfile}"
  fi
  install_name_tool -change "${dep}" "${loaderpath}" "$1"

  if [ -e "${outdir}/${depfile}" ]; then
    error "DEP: ${INSTALLDEPS_INDENT}  ${dep} SKIPPED"
  else
    error "DEP: ${INSTALLDEPS_INDENT}  ${dep} -> ${outdir}"
    cp "${dep}" "${outdir}/"
    chmod u+w "${outdir}/${depfile}"
    install_name_tool -id "${loaderpath}" "${outdir}/${depfile}"
    # recursively call ourselves, to resolve higher-order dependencies
    INSTALLDEPS_INDENT="${INSTALLDEPS_INDENT}    " $0 -r "${outdir}/${depfile}"
  fi
done

}

for f in "${ARGS[@]}"; do
   if [ -e "${f}" ]; then
       install_deps "${f}"
   fi
done

# Code signing
# On Monterey, binaries are automatically codesigned. Modifying them with this script renders the signature
# invalid. When Pd loads an external with an invalid signature, it exits immediately. Thus, we need to make sure
# that we codesign them again _after_ the localdeps process

# This needs to be the absolutely last step. We don't do it while we're still inside a recursion.
if ! $recursion; then
  echo -n "Code signing in progress... "
  codesign --remove-signature "${ARGS[@]}" ${outdir}/*.dylib
  codesign -s -  "${ARGS[@]}" ${outdir}/*.dylib
  echo "Done"
fi
