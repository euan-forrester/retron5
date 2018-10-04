#!/usr/bin/env python3

import sys
import struct
from typing import NamedTuple
import argparse
import zlib

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
RETRON_DATA_HEADER_FORMAT = "I H H I I I I" # The format of this string is described here: https://docs.python.org/3/library/struct.html#struct-format-strings
RETRON_DATA_HEADER_SIZE = struct.calcsize(RETRON_DATA_HEADER_FORMAT)

class RetronDataHeader(NamedTuple):
    magic: int
    formatVersion: int
    flags: int
    originalSize: int
    packedSize: int
    dataOffset: int
    crc32: int

# Command line arguments

parser = argparse.ArgumentParser(description="Read and write Retron5 save files")

parser.add_argument("-d", "--debug", action="store_true", dest="debug", default=False, help="Display debug information")
requiredArguments = parser.add_argument_group('required arguments')
requiredArguments.add_argument("-r", "--retron-file", dest="retronFilename", type=str, help="File in the Retron5 save file format", required=True)

args = parser.parse_args()

# Read file

with open(args.retronFilename, 'rb') as input_file:
	retron_data_header_bytes = input_file.read(RETRON_DATA_HEADER_SIZE)
	save_data_bytes = input_file.read()

	retron_data_header = RetronDataHeader._make(struct.unpack_from(RETRON_DATA_HEADER_FORMAT, retron_data_header_bytes))

input_file.closed

if args.debug:
	print("Read file and found magic 0x%x version 0x%x flags 0x%x originalSize %d packedSize %d data offset %d bytes crc32 0x%x. Header is %d bytes" % (retron_data_header.magic, retron_data_header.formatVersion, retron_data_header.flags, retron_data_header.originalSize, retron_data_header.packedSize, retron_data_header.dataOffset, retron_data_header.crc32, RETRON_DATA_HEADER_SIZE))

# Check file format

if retron_data_header.magic != MAGIC:
	print("Incorrect file format: magic did not match. Got magic 0x%x instead of 0x%x" % (retron_data_header.magic, MAGIC))
	sys.exit(1)

if retron_data_header.formatVersion > FORMAT_VERSION:
	print("Incorrect file format: format version did not match. Got version 0x%x instead of 0x%x" % (retron_data_header.formatVersion, FORMAT_VERSION))

#if retron_data_header.packedSize != len(save_data_bytes):
#	print("Error reading file: expected %d bytes of save data but found %d instead" % (retron_data_header.packedSize, len(save_data_bytes)))

# Pull the save data from the file

if (retron_data_header.flags & FLAG_ZLIB_PACKED) != 0:

	save_data_decompressed = zlib.decompress(save_data_bytes)

	save_data_decompressed_crc32 = zlib.crc32(save_data_decompressed)

	if args.debug:
		print("Decompressed %d bytes into %d bytes; expected to find %d bytes. Found crc32 0x%x; expected 0x%x" % (len(save_data_bytes), len(save_data_decompressed), retron_data_header.originalSize, save_data_decompressed_crc32, retron_data_header.crc32))

	