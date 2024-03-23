#! /bin/bash

# Check if the number of arguments is correct
if [ "$#" -ne 2 ]; then
    echo "Error: Incorrect number of arguments provided."
    echo "Usage: $0 <writefile> <writestr>"
    exit 1
fi

# Extract arguments
writefile="$1"
writestr="$2"

# Check if writefile and writestr are provided
if [ -z "$writefile" ] || [ -z "$writestr" ]; then
    echo "Error: Both writefile and writestr must be specified."
    exit 1
fi

mkdir -p "$(dirname "$writefile")"

# Create the file and write the content
echo "$writestr" > "$writefile"

# Check if the file was created successfully
if [ $? -ne 0 ]; then
    echo "Error: Failed to create or write to '$writefile'."
    exit 1
fi

echo "Content written to '$writefile' successfully."
