#!/bin/bash

MPIENVS=(mpich openmpi)

# build docker containers

i=0
for MPIENV in ${MPIENVS[@]}; do
  if [[ $(( $i % ${CIRCLE_NODE_TOTAL} )) -eq ${CIRCLE_NODE_INDEX} ]]; then
    echo "Pulling docker container $MPIENV"
    docker pull dashproject/ci:$MPIENV
  fi
  i=$((i + 1))
done
