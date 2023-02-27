#!/usr/bin/env bash

HELP="finder.sh DIRECTORY NAME"

filesdir="$1"
searchstr="$2"

test -n "$1" || ( echo "$HELP" ; exit 1 ; ) 
test -n "$2" || ( echo "$HELP" ; exit 1 ; )

test -e "$1" || ( echo "'$1' does not exist" ; exit 1 ; )
test -d "$1" || ( echo "'$1' is not a directory" ; exit 1 ; )

matches=$(grep -r "$2" $1)
lines=$(echo "$matches" | wc -l)
files=$(echo "$matches" | cut -d ':' -f 1 | uniq | wc -l)

echo "The number of files are $files and the number of matching lines are $lines"

