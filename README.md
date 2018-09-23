# retron5
Convert Retron5 save files into files that can be used by other emulators

The Retron5 save format is described here: https://www.retro5.net/viewtopic.php?f=5&t=67&start=10

And specifically the file format is:

```
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
```

It's a header and then a bunch of compressed data, so to unpack the file we need to read past the header and then uncompress the data.
