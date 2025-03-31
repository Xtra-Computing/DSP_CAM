#!/bin/bash
rm -rf ./src/*
cp -r ./template/* ./src

# Load the parameters from param.cfg into an associative array.
declare -A params
while IFS= read -r line || [ -n "$line" ]; do
  # Skip empty lines or lines starting with a comment.
  [[ -z "$line" || "$line" =~ ^# ]] && continue
  
  # Extract the key and value from the line.
  key=$(echo "$line" | cut -d '=' -f1)
  value=$(echo "$line" | cut -d '=' -f2-)
  
  params["$key"]="$value"
done < param.cfg

# First, print out the parameters that will be replaced.
echo "Parameters to be replaced:"
for key in "${!params[@]}"; do
  echo "$key will be replaced with ${params[$key]}"
done

echo "------------------------------------"

# Iterate over every file in the ./src directory.
while IFS= read -r file; do
  for key in "${!params[@]}"; do
    # Replace every occurrence of the parameter word with its value.
    sed -i "s|${key}|${params[$key]}|g" "$file"
  done
  echo "Processed file: $file"
done < <(find ./src -type f)