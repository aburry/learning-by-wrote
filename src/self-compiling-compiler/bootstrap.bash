#!/bin/bash
# Convert a stream of bytes represented as hexidecimal text into actual bytes.
#
# Example: "41 64 61 6d" becomes "Adam".
#
# Usage: compiler1.sh < input.file > output.file
#
# Bytes in the input file are separated by whitespace and ';' is used as
# an inline comment marker.
while read line; do
  # strip comments
  line=${line%%;*}
  # convert bytes
  for byte in $line; do
    echo -n -e "\x$byte"
  done
done
