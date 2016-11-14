#!/bin/bash

MPIENVS=(mpich openmpi openmpi2)

# build docker containers

i=0
for MPIENV in ${MPIENVS[@]}; do
  if [[ $(( $i % ${CIRCLE_NODE_TOTAL} )) -eq ${CIRCLE_NODE_INDEX} ]]; then
    echo "Building docker container $MPIENV"
    docker build --rm=false --tag=dashproject/ci:$MPIENV ./dash/scripts/docker-testing/$MPIENV/
  fi
  i=$((i + 1))
done
