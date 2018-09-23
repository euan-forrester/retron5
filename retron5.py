#!/usr/bin/env python3

import sys
import struct
from typing import NamedTuple

"""
The Retron5 data format is:

typedef struct
{
   uint32_t magic;
   uint16_t fmtVer;
   uint16_t flags;
   uint32_t origSize;
   uint32_t packedSize;
   uint32_t dataOffset;
   uint32_t crc32;
   uint8_t data[0];
} t_retronDataHdr;
"""

class RetronDataHeader(NamedTuple):
    magic: int
    format_version: int
    flags: int
    originalSize: int
    packedSize: int
    crc32: int
    data: int

RetronDataHeaderFormat = "I H H I I I I P" # The format is described here: https://docs.python.org/3/library/struct.html#struct-format-strings

print("Our struct is %d bytes" % (struct.calcsize(RetronDataHeaderFormat)))

if (len(sys.argv) < 2):
	print("Usage: retron5.py <input file>")
	sys.exit(0)	

input_filename = sys.argv[1]

with open(input_filename, 'rb') as input_file:
	input_file_bytes = input_file.read()

	retron_data = RetronDataHeader._make(struct.unpack_from(RetronDataHeaderFormat, input_file_bytes))
input_file.closed