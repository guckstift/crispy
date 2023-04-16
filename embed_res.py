#!/usr/bin/env python3

import sys

def stringify_line(line):
	line = repr(line)[1:-1]
	line = line.replace('"', '\\"')
	line = line.replace('%', '%%')
	return f'"{line}\\n"'

infilename = sys.argv[1]
name = infilename.split("/")[-1].split(".")[0].upper()
outfilename = sys.argv[2]

infile = open(infilename, "r").read()
outfile = open(outfilename, "w")

lines = infile.strip().split("\n")

outfile.write(f"#define {name}_RES \\\n")

output = " \\\n".join(
	'\t' + stringify_line(line) for line in lines
)

outfile.write(output + "\n")

