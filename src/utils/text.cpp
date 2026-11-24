#include "./text.h"

#include <ctype.h>
#include <string.h>

void textTrim(char* text) {
  char* start = text;
  while (*start && isspace((unsigned char)*start)) start++;

  char* end = start + strlen(start);
  while (end > start && isspace((unsigned char)end[-1])) end--;
  *end = '\0';

  if (start != text) memmove(text, start, (size_t)(end - start) + 1);
}

bool textCopyTrimmed(char* dst, size_t dstSize, const char* begin, const char* end) {
  while (begin < end && isspace((unsigned char)*begin)) begin++;
  while (end > begin && isspace((unsigned char)end[-1])) end--;

  const size_t len = (size_t)(end - begin);
  if (len >= dstSize) return false;

  memcpy(dst, begin, len);
  dst[len] = '\0';
  return true;
}

bool textCopyRaw(char* dst, size_t dstSize, const char* begin, const char* end) {
  const size_t len = (size_t)(end - begin);
  if (len >= dstSize) return false;

  memcpy(dst, begin, len);
  dst[len] = '\0';
  return true;
}

void textSanitizeSsid(char* ssid) {
  for (char* pos = ssid; *pos; pos++) {
    if (*pos == '\t' || *pos == '\r' || *pos == '\n') *pos = ' ';
  }
}

// Kept as a general helper; /cmd no longer needs it because AsyncWebServer
// already URL-decodes request parameters.
void textReplaceToken(char* buffer, const char* token, char value) {
  char* pos = buffer;
  while ((pos = strstr(pos, token)) != nullptr) {
    *pos = value;
    memmove(pos + 1, pos + 3, strlen(pos + 3) + 1);
  }
}
