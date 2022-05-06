#!/bin/sh
#
# creates local copies of all dependencies (dynamic libraries)
# and sets RUNPATH to $ORIGIN on each so they will find
# each other.
#
# usage: $0 <binary>
usage() {
    echo "Usage: ${0} <binary> [ <binary2> ... ]"
    echo "   copies the local dependencies of all given binaries besides them"
}

if [ $# -lt 1 ]; then
    usage
    exit 1
fi

# List of libraries that we do not include into our packaging
# becaue we think they will be installed on any system
ld_exclude_list="libc\.so\.*
libarmmem.*\.so.*
libdl\.so.*
libglib-.*\.so.*
libgomp\.so.*
libgthread.*\.so.*
libm\.so.*
libpthread.*\.so.*
libstdc++\.so.*
libgcc_s\.so.*
libpcre\.so.*"

# detect arch
arch=$(uname -m)
case $arch in
    x86_64)
        arch=amd64
        ;;
    i686)
        arch=i386
        ;;
    armv7l)
	arch=arm
esac

error() {
    echo "$@" 1>&2
}

# Check dependencies
cmdlist="awk grep ldd patchelf"
for cmd in $cmdlist; do
    if ! which "${cmd}" > /dev/null; then
        error "Could not find ${cmd}. Is it installed?"
        exit 1
    fi
done

library_in_exclude_list() {
    # arg1: library name
    # returns 0 if arg1 is found in exclude list, otherwise 1
    local libexname="$1"
    skip=1
    set -f
    for expat in $(echo "${ld_exclude_list}"); do
        if echo "$(basename $libexname)" | grep "${expat}" > /dev/null; then
            skip=0
            break
        fi
    done
    set +f
    return $skip
}

make_local_copy_and_set_rpath() {
    # make a local copy of all linked libraries of given binary
    # and set RUNPATH to $ORIGIN (exclude "standard" libraries)
    # arg1: binary to check
    local outdir
    outdir="$(dirname "$1")/${arch}"
    if [ ! -d "${outdir}" ]; then
        outdir=.
    fi
    ldd "$1" | grep ' => ' | while read _ _ libpath _; do
        libname=$(basename "${libpath}")
        outfile="${outdir}/${libname}"
        if library_in_exclude_list "${libname}"; then
            continue
        fi
        if [ ! -e "${libpath}" ]; then
            error "DEP: ${INSTALLDEPS_INDENT}  WARNING: could not make copy of '${libpath}'. Not found"
            continue
        fi
        if [  -e  "${outfile}" ]; then
            error "DEP: ${INSTALLDEPS_INDENT}  ${libpath} SKIPPED"
        else
            error "DEP: ${INSTALLDEPS_INDENT}  ${libpath} -> ${outdir}/"
            cp "${libpath}" "${outfile}"
            patchelf --set-rpath \$ORIGIN "${outfile}"
        fi
    done
    patchelf --set-rpath \$ORIGIN/${arch} "${1}"
}


for binary_file in "$@"; do
    # Check if we can read from given file
    if ! ldd "${binary_file}" > /dev/null 2>&1; then
        error "Skipping '${binary_file}'. Is it a binary file?"
        continue
    fi
    depdir="$(dirname ${binary_file})/${arch}"
    mkdir -p "${depdir}"
    make_local_copy_and_set_rpath $binary_file
done
