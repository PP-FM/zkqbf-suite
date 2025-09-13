#!/bin/bash
# Loop through each file in the <dir> directory.
if [ -z "$1" ]; then
    echo "Usage: $0 <dir>"
    echo "Please provide the directory containing the qdimacs files."
    exit 1
fi

for file in $1/*; do
  # Only process regular files (ignore directories, etc.)
  if [ -f "$file" ]; then
    # Extract the base filename (e.g. "CR.qdimacs" becomes "CR.qdimacs")
    base=$(basename "$file")
    # Remove the extension to get the directory name (e.g. "CR")
    dir="${base%.qdimacs}"
    # Create the directory (if it doesn't already exist) inside <dir>
    mkdir -p "$1/$dir"
    # Move the file into its corresponding directory
    mv "$file" "$1/$dir/"
  fi
done
