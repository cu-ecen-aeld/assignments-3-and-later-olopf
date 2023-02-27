#!/usr/bin/env bash

HELP="writer.sh PATH CONTENT"

path="$1"
content="$2"

test -n "$1" || ( echo "$HELP" ; exit 1 ; )
test -n "$2" || ( echo "$HELP" ; exit 1 ; )

mkdir -p $(dirname $path)
echo "$2" > $path
