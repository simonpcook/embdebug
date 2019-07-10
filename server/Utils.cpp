// GDB Server Utilties: implementation
//
// This file is part of the Embecosm GDB Server.
//
// Copyright (C) 2009-2019 Embecosm Limited
// SPDX-License-Identifier: GPL-3.0-or-later
// ----------------------------------------------------------------------------

#include <cctype>
#include <climits>
#include <cstring>
#include <iostream>

#include "embdebug/Utils.h"

using std::cout;
using std::endl;
using std::hex;
using std::string;
using std::vector;

using namespace EmbDebug;

bool Utils::isHexStr(const char *buf, const std::size_t len) {
  if (len < 1)
    return false;

  for (std::size_t i = 0; i < len; i++)
    if (!isxdigit(buf[i]))
      return false;

  return true;
}

uint8_t Utils::char2Hex(int c) {
  return ((c >= 'a') && (c <= 'f'))
             ? c - 'a' + 10
             : ((c >= '0') && (c <= '9'))
                   ? c - '0'
                   : ((c >= 'A') && (c <= 'F')) ? c - 'A' + 10 : -1;
}

char Utils::hex2Char(uint8_t d) {
  static const char map[] = "0123456789abcdef"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                            "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

  return map[d];
}

void Utils::regVal2Hex(uint64_t val, char *buf, std::size_t numBytes,
                       bool isLittleEndianP) {
  if (isLittleEndianP) {
    for (std::size_t n = 0; n < numBytes; n++) {
      unsigned char byte = val & 0xff;

      buf[n * 2] = hex2Char((byte >> 4) & 0xf);
      buf[n * 2 + 1] = hex2Char(byte & 0xf);

      val = val / 256;
    }
  } else {
    for (std::size_t n = numBytes; n-- > 0;) {
      unsigned char byte = val & 0xff;

      buf[n * 2] = hex2Char((byte >> 4) & 0xf);
      buf[n * 2 + 1] = hex2Char(byte & 0xf);

      val = val / 256;
    }
  }

  buf[numBytes * 2] = '\0'; // Useful to terminate as string
}

uint64_t Utils::hex2RegVal(const char *buf, std::size_t numBytes,
                           bool isLittleEndianP) {
  uint64_t val = 0; // The result

  if (isLittleEndianP) {
    for (std::size_t n = numBytes; n-- > 0;) {
      val = (val << 4) | char2Hex(buf[n * 2]);
      val = (val << 4) | char2Hex(buf[n * 2 + 1]);
    }
  } else {
    for (std::size_t n = 0; n < numBytes; n++) {
      val = (val << 4) | char2Hex(buf[n * 2]);
      val = (val << 4) | char2Hex(buf[n * 2 + 1]);
    }
  }

  return val;
}

std::size_t Utils::val2Hex(uint64_t val, char *buf) {
  int numChars = 0;

  // This will do it back to front

  do {
    buf[numChars] = hex2Char(val & 0xf);
    val = val >> 4;
    numChars++;
  } while (val != 0);

  // Now reverse the string and null terminate

  for (int i = 0; i < numChars / 2; i++) {
    char tmp = buf[numChars - i - 1];
    buf[numChars - i - 1] = buf[i];
    buf[i] = tmp;
  }

  buf[numChars] = '\0';
  return strlen(buf);
}

uint64_t Utils::hex2Val(const char *buf, std::size_t len) {
  uint64_t val = 0; // The result

  for (std::size_t i = 0; i < len; i++)
    val = (val << 4) | char2Hex(buf[i]);

  return val;
}

void Utils::ascii2Hex(char *dest, const char *src) {
  int i;

  // Step through converting the source string
  for (i = 0; src[i] != '\0'; i++) {
    char ch = src[i];

    dest[i * 2] = hex2Char(ch >> 4 & 0xf);
    dest[i * 2 + 1] = hex2Char(ch & 0xf);
  }

  dest[i * 2] = '\0';
}

void Utils::hex2Ascii(char *dest, const char *src) {
  int i;

  // Step through convering the source hex digit pairs
  for (i = 0; src[i * 2] != '\0' && src[i * 2 + 1] != '\0'; i++) {
    dest[i] =
        ((char2Hex(src[i * 2]) & 0xf) << 4) | (char2Hex(src[i * 2 + 1]) & 0xf);
  }

  dest[i] = '\0';
}

std::size_t Utils::rspUnescape(char *buf, std::size_t len) {
  std::size_t fromOffset = 0; // Offset to source char
  std::size_t toOffset = 0;   // Offset to dest char

  while (fromOffset < len) {
    // Is it escaped
    if ('}' == buf[fromOffset]) {
      fromOffset++;
      buf[toOffset] = buf[fromOffset] ^ 0x20;
    } else {
      buf[toOffset] = buf[fromOffset];
    }

    fromOffset++;
    toOffset++;
  }

  return toOffset;
}

vector<string> &Utils::split(const string &s, const string &delim,
                             vector<string> &elems) {
  elems.clear();
  std::size_t current;
  std::size_t next = s.npos;

  do {
    next = s.find_first_not_of(delim, next + 1);
    if (next == s.npos) {
      break;
    }
    current = next;
    next = s.find_first_of(delim, current);
    elems.push_back(s.substr(current, next - current));
  } while (next != s.npos);

  return elems;
}

bool Utils::str2int(int &i, const std::string &str, int base) {
  const char *s;
  char *end;
  long val;

  s = str.c_str();
  errno = 0;
  val = strtol(s, &end, base);
  if ((errno == ERANGE && val == LONG_MAX) || val > INT_MAX)
    return false; /* Overflow.  */

  if ((errno == ERANGE && val == LONG_MIN) || val < INT_MIN)
    return false; /* Underflow.  */

  if (*s == '\0' || *end != '\0')
    return false; /* Conversion failed.  */

  i = val;
  return true;
}

void Utils::fatalError(std::string str) {
  std::cerr << "*** FATAL ERROR: " << str << std::endl;
  abort();
}
