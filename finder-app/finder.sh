#!/bin/bash

if [ ! $# -eq 2 ]; then
	echo "Error: Invalid Arguments."
	echo "Total number or arguments should be 2."
	echo -e "The order of arguments should be:\n\t1) File Directoy Path\n\t2) String to be searched in the specified directory path"
	exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d $filesdir ]; then
	echo $filesdir directory does not exist
	exit 1
fi


echo searching string $searchstr in directory $filesdir

numfiles=$(ls $filesdir | wc -l)
matchinglines=$(grep -r $searchstr $filesdir | wc -l)

echo "The number of files are $numfiles and the number of matching lines are $matchinglines"
