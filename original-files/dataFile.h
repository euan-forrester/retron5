#ifndef _DATAFILE_H
#define _DATAFILE_H

#define RETRON_DATA_MAGIC         (0x354E5452)   // "RTN5"
#define RETRON_DATA_FORMAT_VER      (1)
#define RETRON_DATA_TMP_POSTFIX      ".TMP"

#define RETRON_DATA_FLG_ZLIB_PACKED   (0x01)

#define RETRON_DATA_DEFCOMPRESS      (true)
#define RETRON_DATA_RAMFILE         "/mnt/ram/tempdata.bin"

enum
{
   RETRON_DATA_SUCCESS = 0,
   RETRON_DATA_ERR_CORRUPT = -1,
   RETRON_DATA_ERR_IO = -2,
   RETRON_DATA_ERR_MEM = -3,
   RETRON_DATA_ERR_VERSION = -4,
   RETRON_DATA_ERR_MISC = -5,
};

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

class cRetronData
{
public:

   cRetronData(const char *dataFilename);
   ~cRetronData();
   int write(void *srcBuf, int srcBufSize, bool compress); // write from buffer into datafile
   int write(const char *srcFilename, bool compress); // write from normal file into datafile
   int read(void **destBuf, int *destBufSize); // read from datafile into buffer
   int read(const char *destFilename); // read from datafile into normal file

private:

   bool readFile(const char *filename, void **buffer, int *size);
   bool writeFile(const char *filename, void *buffer, int size);
   bool flushFs(const char *filename);
   int read(FILE *dataFd, void **destBuf, int *destBufSize);

   const char *mDataFilename, *mDataTmpFilename;
};

#endif