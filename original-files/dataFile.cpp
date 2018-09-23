#include "retronEngine.h"

/*
 * Retron data file safety wrapper - protects data files (such as snapshots, sram states) with CRC32 and ensures FS integrity
 * by first writing to a temp file then switching via rename at the end, and flushing FS after use. Data files are optionally compressed
 */

#define FREAD_SAFE(ptr, bytes, fd)   { int _n = fread((ptr), 1, (bytes), (fd)); if(_n != (bytes)) { rv = RETRON_DATA_ERR_IO; goto end; } }
#define FWRITE_SAFE(ptr, bytes, fd)   { int _n = fwrite((ptr), 1, (bytes), (fd)); if(_n != (bytes)) { rv = RETRON_DATA_ERR_IO; goto end; } }
#define FSEEK_SAFE(fd, off, whence)   { int _o = fseek((fd), (off), (whence)); if(_o < 0) { rv = RETRON_DATA_ERR_IO; goto end; } \
                              if(((whence) == SEEK_SET) && (ftell((fd)) != (off))) { rv = RETRON_DATA_ERR_IO; goto end; } }
#define BAIL(code)               { rv = code; goto end; }

cRetronData::cRetronData(const char *dataFilename)
{
   mDataFilename = strdup(dataFilename);
   assert(dataFilename != NULL);
   int tmpLen = strlen(mDataFilename) + strlen(RETRON_DATA_TMP_POSTFIX) + 1;
   char *tmpName = (char *)malloc(tmpLen);
   assert(tmpName != NULL);
   strcpy(tmpName, (char *)mDataFilename);
   strcat(tmpName, RETRON_DATA_TMP_POSTFIX);
   mDataTmpFilename = tmpName;
}

cRetronData::~cRetronData()
{
   if(mDataFilename)
      free((void *)mDataFilename);
   if(mDataTmpFilename)
      free((void *)mDataTmpFilename);
}

int cRetronData::write(void *srcBuf, int srcBufSize, bool compress)
{
   t_retronDataHdr hdr;
   FILE *fd = NULL;
   int rv = RETRON_DATA_ERR_IO;
   Bytef *destBuf = NULL;

   memset(&hdr, 0, sizeof(hdr));
   hdr.magic = RETRON_DATA_MAGIC;
   hdr.fmtVer = RETRON_DATA_FORMAT_VER;
   hdr.dataOffset = sizeof(hdr);
   hdr.crc32 = crc32(0, (const Bytef *)srcBuf, srcBufSize);
   hdr.origSize = srcBufSize;

   fd = fopen(mDataTmpFilename, "wb+");
   if(!fd)
      BAIL(RETRON_DATA_ERR_IO);

   if(compress)
   {
      hdr.flags |= RETRON_DATA_FORMAT_VER;

      uLongf destLen = compressBound(srcBufSize);
      destBuf = (Bytef *)malloc(destLen);
      if(!destBuf)
         BAIL(RETRON_DATA_ERR_MEM);
      rv = compress2(destBuf, &destLen, (const Bytef *)srcBuf, srcBufSize, Z_DEFAULT_COMPRESSION);
      if(rv != Z_OK)
         BAIL(RETRON_DATA_ERR_MISC);

      hdr.packedSize = destLen;
      FWRITE_SAFE(&hdr, sizeof(hdr), fd);
      FWRITE_SAFE(destBuf, destLen, fd);
   }
   else
   {
      FWRITE_SAFE(&hdr, sizeof(hdr), fd);
      FWRITE_SAFE(srcBuf, srcBufSize, fd);
   }

   fclose(fd);
   fd = NULL;
   if(flushFs(mDataTmpFilename) != true)
      BAIL(RETRON_DATA_ERR_IO);
   // at this point the filesystem should have a clean copy of the datafile at the temp location

   // only try to remove it file already exists
   struct stat st;
   if(lstat(mDataFilename, &st) >= 0)
   {
      if(remove(mDataFilename) < 0)
      {
         LOGI("bail 1 %d\n", rv);
         BAIL(RETRON_DATA_ERR_IO);
      }
   }
   if(rename(mDataTmpFilename, mDataFilename) < 0)
   {
      LOGI("bail 2\n");
      BAIL(RETRON_DATA_ERR_IO);
   }
   if(flushFs(mDataTmpFilename) != true)
      BAIL(RETRON_DATA_ERR_IO);

   // at this point, the old file has been removed and the temp file renamed, and everything flushed - we're done
   rv = RETRON_DATA_SUCCESS;

end:
   if(fd)
      fclose(fd);
   if(destBuf)
      free(destBuf);
   LOGI("datafile write rv: %d\n", rv);
   return rv;
}

// TODO: implement proper streaming code, so we don't need to read the entire file in first
int cRetronData::write(const char *srcFilename, bool compress)
{
   void *buffer = NULL;
   int fileSize = 0;
   if(readFile(srcFilename, &buffer, &fileSize) != true)
   {
      LOGE("retron dat failed to read file\n");
      return RETRON_DATA_ERR_IO;
   }
   int rv = write(buffer, fileSize, compress);
   free(buffer);
   return rv;
}

int cRetronData::read(FILE *dataFd, void **destBuf, int *destBufSize)
{
   t_retronDataHdr hdr;
   int rv = RETRON_DATA_ERR_IO;
   uint32_t thisCRC;
   Bytef *compBuf = NULL;

   FREAD_SAFE(&hdr, sizeof(hdr), dataFd);
   if(hdr.magic != RETRON_DATA_MAGIC)
      BAIL(RETRON_DATA_ERR_CORRUPT);

   // cant operate on files with newer version
   if(hdr.fmtVer > RETRON_DATA_FORMAT_VER)
      BAIL(RETRON_DATA_ERR_VERSION);

   *destBufSize = hdr.origSize;
   *destBuf = (void *)malloc(*destBufSize);
   if(*destBuf == NULL)
      BAIL(RETRON_DATA_ERR_MEM);

   if(hdr.flags & RETRON_DATA_FLG_ZLIB_PACKED)
   {
      compBuf = (Bytef *)malloc(hdr.packedSize);
      uLongf destLen = hdr.origSize;
      if(!compBuf)
         BAIL(RETRON_DATA_ERR_MEM);
      FSEEK_SAFE(dataFd, hdr.dataOffset, SEEK_SET);
      FREAD_SAFE(compBuf, hdr.packedSize, dataFd);
      rv = uncompress((Bytef *)*destBuf, &destLen, compBuf, hdr.packedSize);
      if(rv != Z_OK)
         BAIL(RETRON_DATA_ERR_CORRUPT);
      if(destLen != *destBufSize)
         BAIL(RETRON_DATA_ERR_CORRUPT);
   }
   else
   {
      FSEEK_SAFE(dataFd, hdr.dataOffset, SEEK_SET);
      FREAD_SAFE(*destBuf, *destBufSize, dataFd);
   }

   thisCRC = crc32(0, (const Bytef *)*destBuf, *destBufSize);
   if(thisCRC != hdr.crc32)
   {
      LOGE("datafile crc mismatch: %08X, %08X\n", thisCRC, hdr.crc32);
      BAIL(RETRON_DATA_ERR_CORRUPT);
   }

   rv = RETRON_DATA_SUCCESS;

end:
   if(compBuf)
      free(compBuf);
   LOGI("datafile read rv: %d\n", rv);
   return rv;
}

int cRetronData::read(void **destBuf, int *destBufSize)
{
   FILE *fd;
   int rv = RETRON_DATA_ERR_IO;

   // first try to read from temp file.. if this succeeds then it indicates a power outage or filesystem error on the last save
   fd = fopen(mDataTmpFilename, "rb");
   if(fd)
   {
      LOGE("WARNING: temp datafile left over!\n");
      rv = read(fd, destBuf, destBufSize);
      fclose(fd);
      if(rv == RETRON_DATA_SUCCESS)
         return rv;
   }

   // next try to read from the normal data file
   fd = fopen(mDataFilename, "rb");
   if(!fd)
   {
      LOGE("fopen failed: %s\n", mDataFilename);
      return RETRON_DATA_ERR_IO;
   }
   rv = read(fd, destBuf, destBufSize);
   fclose(fd);
   return rv;
}

int cRetronData::read(const char *destFilename)
{
   void *buffer = NULL;
   int fileSize = 0;
   int rv = read(&buffer, &fileSize);
   if(rv != RETRON_DATA_SUCCESS)
   {
      if(buffer)
         free(buffer);
      return rv;
   }
   rv = ((writeFile(destFilename, buffer, fileSize) == true) ? RETRON_DATA_SUCCESS : RETRON_DATA_ERR_IO);
   free(buffer);
   return rv;
}

bool cRetronData::readFile(const char *filename, void **buffer, int *size)
{
   FILE *fd = fopen(filename, "rb");
   if(!fd)
   {
      LOGE("readFile fopen failed: %d, %d\n", fd, errno);
      return false;
   }
   fseek(fd, 0, SEEK_END);
   *size = ftell(fd);
   fseek(fd, 0, SEEK_SET);
   *buffer = (void *)malloc(*size);
   assert(*buffer != NULL);
   int rv = fread(*buffer, *size, 1, fd);
   fclose(fd);
   if(rv != 1)
   {
      LOGE("readFile fread failed: %d, %d\n", rv, errno);
      free(*buffer);
      return false;
   }

   return true;
}

bool cRetronData::writeFile(const char *filename, void *buffer, int size)
{
   FILE *fd = fopen(filename, "wb+");
   if(!fd)
      return false;
   int rv = fwrite(buffer, size, 1, fd);
   fclose(fd);
   if(rv != 1)
      return false;

   return flushFs(filename);
}

bool cRetronData::flushFs(const char *filename)
{
   sync();
   return true;
}