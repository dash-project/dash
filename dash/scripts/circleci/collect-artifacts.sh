#!/bin/bash

[[ "$CIRCLECI" == "true" ]] || { echo "Not running on CircleCI"; exit 1; }

mkdir -p /tmp/build-logs/${CIRCLE_JOB}
mkdir -p /tmp/build-logs/${CIRCLE_JOB}/tests

cd build-ci

echo "Collect Logs"
cp -v --parents -R ./*/*/*/*.log /tmp/build-logs/${CIRCLE_JOB}

echo "Collect Tests"
cp -v --parents -R ./*/*/*/dash-tests-*.xml /tmp/build-logs/${CIRCLE_JOB}/tests

echo "Artifacts copied to /tmp/build-logs/${CIRCLE_JOB}"

