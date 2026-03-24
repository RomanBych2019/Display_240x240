#include "PNG_FS_Support.h"

static File g_pngFile;

void *pngOpen(const char *filename, int32_t *size)
{
    g_pngFile = LittleFS.open(filename, "r");
    if (!g_pngFile)
    {
        *size = 0;
        return nullptr;
    }

    *size = g_pngFile.size();
    return &g_pngFile;
}

void pngClose(void *handle)
{
    File *file = static_cast<File *>(handle);
    if (file && *file)
    {
        file->close();
    }
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length)
{
    File *file = static_cast<File *>(page->fHandle);
    if (!file || !(*file))
        return 0;

    return file->read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position)
{
    File *file = static_cast<File *>(page->fHandle);
    if (!file || !(*file))
        return 0;

    return file->seek(position);
}