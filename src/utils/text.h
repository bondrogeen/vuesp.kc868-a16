#ifndef UTILS_TEXT_H
#define UTILS_TEXT_H

#include <stddef.h>

void textTrim(char* text);
bool textCopyTrimmed(char* dst, size_t dstSize, const char* begin, const char* end);
bool textCopyRaw(char* dst, size_t dstSize, const char* begin, const char* end);
void textSanitizeSsid(char* ssid);
void textReplaceToken(char* buffer, const char* token, char value);

#endif
