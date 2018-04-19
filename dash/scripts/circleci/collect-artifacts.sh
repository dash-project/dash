#!/bin/bash
# Copy Build artifacts to tmp directory to collect them later using circleci

[[ "$CIRCLECI" == "true" ]] || { echo "Not running on CircleCI"; exit 1; }

mkdir -p /tmp/build-logs/${CIRCLE_JOB}
mkdir -p /tmp/build-tests/${CIRCLE_JOB}

cd build-ci

echo "Collect Logs"
cp -v --parents -R ./*/*/*/*.log /tmp/build-logs/${CIRCLE_JOB}
echo "Artifacts copied to /tmp/build-logs/${CIRCLE_JOB}"

echo "Collect Tests"
cp -v --parents -R ./*/*/*/dash-tests-*.xml /tmp/build-tests/${CIRCLE_JOB}
echo "Testlogs copied to /tmp/build-tests/${CIRCLE_JOB}"

