#!/bin/sh
# first we need this statement above

# check if the number of arguments is correct using the $# variable
if [ "$#" -ne 2 ]; then
	echo "Error: Incorrect number of arguments provided."
	echo "Usage: $0 <filesdir> <searchstr>"
	exit 1
fi

# extract arguments
filesdir="$1"
searchstr="$2"

# check if filesdir exists and is a directory
if [ ! -d "$filesdir" ]; then
	echo "Error: '$filesdir' is not a directory."
	exit 1
fi

# now we can do the work
num_files=$(find "$filesdir" -type f | wc -l)
num_matching_lines=$(grep -r "$searchstr" "$filesdir" | wc -l)

# print the results
echo "The number of files are $num_files and the number of matching lines are $num_matching_lines"
