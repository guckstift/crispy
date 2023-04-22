#!/usr/bin/env python3

import sys

infilename = sys.argv[1]
name = infilename.split("/")[-1].split(".")[0].upper()
outfilename = sys.argv[2]

infile = open(infilename, "rb").read()
outfile = open(outfilename, "w")

outfile.write("static const char " + name + "_RES[] = {\n")

i = 0
for c in infile:
	c = chr(c)
	outfile.write(repr(c) + ',')
	i += 1
	if i % 16 == 0:
		outfile.write("\n")

outfile.write("0};\n")
