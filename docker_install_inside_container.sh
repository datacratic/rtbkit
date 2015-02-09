#!/bin/bash

set -e

echo "installing from " $1

cd $1
cp -av * /

$2
