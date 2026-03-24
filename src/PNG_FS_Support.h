#pragma once

#include "Arduino.h"
#include <LittleFS.h>
#include <PNGdec.h>

// callbacks for PNGdec
void *pngOpen(const char *filename, int32_t *size);
void pngClose(void *handle);
int32_t pngRead(PNGFILE *page, uint8_t *buffer, int32_t length);
int32_t pngSeek(PNGFILE *page, int32_t position);