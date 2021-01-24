#!/bin/bash
cd "$(dirname "${BASH_SOURCE[0]}")"
while true;
do
  ./redis-camera-client
done
