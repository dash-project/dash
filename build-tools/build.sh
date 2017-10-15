#!/bin/sh

# traverse symlinks of the script and find the original scriptdir
find_scriptdir() {
  (
    SCRIPT=$1
    while [ -h $SCRIPT ]
    do
      BASE=$(dirname $SCRIPT)
      SCRIPT=$(readlink $SCRIPT)
      cd $BASE
    done

    cd $(dirname $SCRIPT) && pwd -P
  )
}

# find the root of the project
# when given the scriptdir as parameter
find_rootdir() {
  ( cd $1/.. && pwd -P )
}

get_first_char() {
  head -n 1 | cut -c 1
}

get_remainder() {
  head -n 1 | cut -c 2-
}

confirm_purge() {
  if $PURGE ; then
    true; return $?
  fi
  if $FORCE ; then
    false; return $?
  fi

  echo "   Purge build directory before building? [y/N]"
  local CONF=`get_first_char`
  if [ "$CONF" = "y" -o "$CONF" = "Y" ]; then
    true; return $?
  else
    false; return $?
  fi
}

confirm_build() {
  if ! $BUILD ; then
    echo "To build DASH, call \"$MAKE_COMMAND\" in $BUILD_DIR"
    false; return $?
  fi
  if $FORCE ; then
    true; return $?
  fi

  echo ""
  echo "   Build with these settings? [Y/n]"
  local CONF=`get_first_char`
  if [ "$CONF" = "n" -o "$CONF" = "N" ]; then
    echo "To build DASH, call \"$MAKE_COMMAND\" in $BUILD_DIR"
    false; return $?
  else
    true; return $?
  fi
}

confirm_install() {
  if $INSTALL ; then
    true; return $?
  fi
  if $FORCE ; then
    echo "To install DASH, run \"$INSTALL_COMMAND\" in $BUILD_DIR"
    false; return $?
  fi

  echo ""
  echo "   Done. Install DASH now? [y/N]"
  local CONF=`get_first_char`
  if [ "$CONF" = "y" -o "$CONF" = "Y" ]; then
    true; return $?
  else
    echo "To install DASH, run \"$INSTALL_COMMAND\" in $BUILD_DIR"
    false; return $?
  fi
}

print_help() {
  echo "Usage: $0 [-f] [-p] [-n] [-i] [-h] [build type]"
  echo "  -f --force    do not ask back for confirmation"
  echo "  -p --purge    purge the build directory before building"
  echo "  -n --nobuild  just configure the project with cmake"
  echo "  -i --install  install the project"
  echo "  -h --help     display this help and exit"
  echo "  [build type]  specify which configuration to use,"
  echo "                see \"[my.]build.<type>.sh\" scripts"
  exit 0
}

parse_params() {
  local PAR=$(echo "$1" | get_first_char)
  local REM=$(echo "$1" | get_remainder)

  if [ "$PAR" = "f" ]; then
    FORCE=true
  elif [ "$PAR" = "p" ]; then
    PURGE=true
  elif [ "$PAR" = "n" ]; then
    BUILD=false
  elif [ "$PAR" = "i" ]; then
    INSTALL=true
  elif [ "$PAR" = "h" ]; then
    print_help
  else
    echo "Unkown parameter" 1>&2
    exit 1
  fi

  if [ -n "$REM" ]; then
    parse_params $REM
  fi
}

SCRIPTDIR=$(find_scriptdir $0)
ROOTDIR=$(find_rootdir $SCRIPTDIR)

SOURCING=true
# load default options
. $SCRIPTDIR/build.default.sh

FORCE=false
PURGE=false
BUILD=true
INSTALL=false
# go through parameters
while [ $# -gt 0 ]; do
  if [ "$1" = "--force" ]; then
    FORCE=true
  elif [ "$1" = "--purge" ]; then
    PURGE=true
  elif [ "$1" = "--nobuild" ]; then
    BUILD=false
  elif [ "$1" = "--install" ]; then
    INSTALL=true
  elif [ "$1" = "--help" ]; then
    print_help
  elif [ "$(printf "%s" "$1" | get_first_char)" = "-" ]; then
    parse_params $(printf "%s" "$1" | get_remainder)
  else
    # load options from seperate file(s)
    if [ -x $SCRIPTDIR/build.$1.sh ]; then
      . $SCRIPTDIR/build.$1.sh
    fi
    if [ -x $SCRIPTDIR/my.build.$1.sh ]; then
      . $SCRIPTDIR/my.build.$1.sh
    fi
  fi

  shift
done


if [ "${PAPI_HOME}" = "" ]; then
  PAPI_HOME=$PAPI_BASE
fi

# Configure with build settings loaded previously:
mkdir -p $ROOTDIR/$BUILD_DIR
if confirm_purge ; then
  rm -Rf $ROOTDIR/$BUILD_DIR/*
fi
(cd $ROOTDIR/$BUILD_DIR && $CMAKE_COMMAND $CMAKE_OPTIONS \
                                          $ROOTDIR && \
 if confirm_build ; then BUILT=true ; $MAKE_COMMAND ; else BUILT=false ; fi && \
 if $BUILT && confirm_install ; then $INSTALL_COMMAND ; fi)

