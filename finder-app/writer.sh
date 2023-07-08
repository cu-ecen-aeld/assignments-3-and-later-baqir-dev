#!/bin/bash

if [ ! $# -eq 2 ]; then
	echo "Error: Invalid Arguments."
	echo "Total number or arguments should be 2."
	echo -e "The order of arguments should be:\n\t1) File Directory Path\n\t2) String to be written in the specified file"
	exit 1
fi

writefile=$1
writestr=$2

mkdir -p "$(dirname "$writefile")" && touch "$writefile" && echo $writestr > $writefile

if [ $? -eq 0 ]; then
	exit 0
else
	exit 1
fi
