#!/usr/bin/env python3

import sys
import struct
from typing import NamedTuple
import argparse

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

MAGIC = 0x354E5452 # "RTN5"
FORMAT_VERSION = 1
FLAG_ZLIB_PACKED = 0x01
RETRON_DATA_HEADER_FORMAT = "I H H I I I P" # The format of this string is described here: https://docs.python.org/3/library/struct.html#struct-format-strings

class RetronDataHeader(NamedTuple):
    magic: int
    formatVersion: int
    flags: int
    originalSize: int
    packedSize: int
    crc32: int
    data: int

parser = argparse.ArgumentParser(description="Read and write Retron5 save files")

parser.add_argument("-d", "--debug", action="store_true", dest="debug", default=False, help="Display debug information")
requiredArguments = parser.add_argument_group('required arguments')
requiredArguments.add_argument("-r", "--retron-file", dest="retronFilename", type=str, help="File in the Retron5 save file format", required=True)

args = parser.parse_args()

with open(args.retronFilename, 'rb') as input_file:
	input_file_bytes = input_file.read()

	retron_data = RetronDataHeader._make(struct.unpack_from(RETRON_DATA_HEADER_FORMAT, input_file_bytes))

input_file.closed

if retron_data.magic != MAGIC:
	print("Incorrect file format: magic did not match. Got magic %x instead of %x" % (retron_data.magic, MAGIC))
	sys.exit(1)

if args.debug:
	print("Read file and found magic 0x%x version 0x%x flags 0x%x originalSize %d packedSize %d crc32 0x%x" % (retron_data.magic, retron_data.formatVersion, retron_data.flags, retron_data.originalSize, retron_data.packedSize, retron_data.crc32))