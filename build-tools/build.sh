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

await_confirm() {
  if ! $FORCE_BUILD; then
    echo ""
    echo "   To build using these settings, hit ENTER"
    read confirm
  fi
}

exit_message() {
  echo "--------------------------------------------------------"
  echo "Done. To install DASH, run    make install    in $BUILD_DIR"
}

SCRIPTDIR=$(find_scriptdir $0)
ROOTDIR=$(find_rootdir $SCRIPTDIR)

SOURCING=true
# load default options
. $SCRIPTDIR/build.default.sh

FORCE_BUILD=false
# go through parameters
while [ $# -gt 0 ]; do
  if [ "$1" = "-f" ]; then
    FORCE_BUILD=true
  fi

  # load options from seperate file(s)
  if [ -x $SCRIPTDIR/build.$1.sh ]; then
    . $SCRIPTDIR/build.$1.sh
  fi
  if [ -x $SCRIPTDIR/my.build.$1.sh ]; then
    . $SCRIPTDIR/my.build.$1.sh
  fi

  shift
done


if [ "${PAPI_HOME}" = "" ]; then
  PAPI_HOME=$PAPI_BASE
fi

# Configure with build settings loaded previously:
mkdir -p $ROOTDIR/$BUILD_DIR
rm -Rf $ROOTDIR/$BUILD_DIR/*
(cd $ROOTDIR/$BUILD_DIR && cmake $CMAKE_OPTIONS \
                                 $ROOTDIR && \
 await_confirm && \
 $MAKE_COMMAND) && \
exit_message

