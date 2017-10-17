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

check_install() {
  if $INSTALL ; then
    true; return $?
  fi

  echo "Done. To install DASH, call this script with \"--install\""
  echo "      or run \"$INSTALL_COMMAND\" in $BUILD_DIR"
  false; return $?
}

list_build_types() {
  echo "Available build types:"
  for SCR in `(cd $SCRIPTDIR && ls)`; do
    echo $SCR | sed -n "s/.*build.\(.*\).sh/  \1/p"
  done | sort | uniq
  exit 0
}

edit_build_type() {
  if [ ! -e $SCRIPTDIR/my.build.$1.sh ]; then
    if [ -e $SCRIPTDIR/build.$1.sh ]; then
      cp $SCRIPTDIR/build.$1.sh $SCRIPTDIR/my.build.$1.sh
    else
      cp $SCRIPTDIR/build.default.sh $SCRIPTDIR/my.build.$1.sh
    fi
  fi

  if [ ! -z $VISUAL ]; then
    exec $VISUAL $SCRIPTDIR/my.build.$1.sh
  elif [ ! -z $EDITOR ]; then
    exec $EDITOR $SCRIPTDIR/my.build.$1.sh
  else
    echo "No editor found, set \"\$VISUAL\" or \"\$EDITOR\" to your default editor"
    echo "The configuration is written to $SCRIPTDIR/my.build.$1.sh"
    exit 1
  fi
  exit 0
}

print_help() {
  echo "Usage: $0 [-f] [-p] [-n] [-i] [-h] [-l] [-e <t>] [build type]"
  echo "  -f --force    do not ask back for confirmation"
  echo "  -p --purge    purge the build directory before building"
  echo "  -n --nobuild  just configure the project with cmake"
  echo "  -i --install  install the project"
  echo "  -h --help     display this help and exit"
  echo "  -l --list     list available build types and exit"
  echo "  -e --edit <t> create and edit build type <t>"
  echo "  [build type]  specify which configuration to use,"
  echo "                see \"--list\" for available types"
  exit 0
}

SCRIPTDIR=$(find_scriptdir $0)
ROOTDIR=$(find_rootdir $SCRIPTDIR)

EDIT=false
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
  elif [ "$PAR" = "l" ]; then
    list_build_types
  elif [ "$PAR" = "e" ]; then
    EDIT=true
    return
  else
    echo "Unkown parameter" 1>&2
    exit 1
  fi

  if [ -n "$REM" ]; then
    parse_params $REM
  fi
}

FORCE=false
PURGE=false
BUILD=true
INSTALL=false

SOURCING=true
# load default options
. $SCRIPTDIR/build.default.sh
if [ -x $SCRIPTDIR/my.build.default.sh ]; then
  . $SCRIPTDIR/my.build.default.sh
fi

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
  elif [ "$1" = "--list" ]; then
    list_build_types
  elif [ "$1" = "--edit" ]; then
    EDIT=true
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

  if $EDIT ; then
    if [ -z "$2" ]; then
      echo "Specify the configuration type you want to edit."
      exit 1
    elif [ "$(printf "%s" "$2" | get_first_char)" = "-" ]; then
      echo "The name of a configuration cannot start with '-'"
      exit 1
    else
      edit_build_type $2
    fi
  fi

  shift
done


if [ "${PAPI_HOME}" = "" ]; then
  PAPI_HOME=$PAPI_BASE
fi

# Configure with build settings loaded previously:
mkdir -p $ROOTDIR/$BUILD_DIR
if $PURGE ; then
  rm -Rf $ROOTDIR/$BUILD_DIR/*
fi
(cd $ROOTDIR/$BUILD_DIR && $CMAKE_COMMAND $CMAKE_OPTIONS \
                                          $ROOTDIR && \
 if confirm_build ; then BUILT=true ; $MAKE_COMMAND ; else BUILT=false ; fi && \
 if $BUILT && check_install ; then $INSTALL_COMMAND ; fi)

