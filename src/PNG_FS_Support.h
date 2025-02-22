// Here are the callback functions that the decPNG library
// will use to open files, fetch data and close the file.
#pragma once
#include "Arduino.h"
#include <PNGdec.h>
#include "SPI.h"
#include <TFT_eSPI.h> 
#include "TFT_240_240.h"

#define FileSys LittleFS

PNG png;
File pngfile;

void *pngOpen(const char *filename, int32_t *size)
{
    pngfile = FileSys.open(filename, "r");
    *size = pngfile.size();
    return &pngfile;
}

void pngClose(void *handle)
{
    File pngfile = *((File *)handle);
    if (pngfile)
        pngfile.close();
}

int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length)
{
    if (!pngfile)
        return 0;
    page = page; 
    return pngfile.read(buffer, length);
}

int32_t pngSeek(PNGFILE *page, int32_t position)
{
    if (!pngfile)
        return 0;
    page = page; 
    return pngfile.seek(position);
}