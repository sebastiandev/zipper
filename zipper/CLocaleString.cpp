// Copyright (C) 2011 - 2015 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc., University of Heidelberg, and The University
// of Manchester.
// All rights reserved.

#include "CLocaleString.h"

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
# include <windows.h>
# define strdup _wcsdup
#endif // WIN32

#if (defined SunOS || defined Linux)
# include <errno.h>
# include <iconv.h>
# include <langinfo.h>
#endif // SunOS || Linux

#if (defined SunOS || defined Linux)
const char * findLocale()
{
  static char * Locale = NULL;

  if (Locale == NULL)
    Locale = strdup(nl_langinfo(CODESET));

#ifdef SunOS

  if (strcmp(Locale, "646") == 0)
    pfree(Locale);

  if (Locale == NULL)
    Locale = strdup("8859-1");

#else

  if (Locale == NULL)
    Locale = strdup("ISO-8859-1");

#endif

  return Locale;
}
#endif // SunOS || Linux


using namespace zipper;

// static
CLocaleString CLocaleString::fromUtf8(const std::string & utf8)
{
#ifdef WIN32
  int size;

  size = MultiByteToWideChar(CP_UTF8, // code page
                             MB_ERR_INVALID_CHARS, // character-type options
                             utf8.c_str(), // address of string to map
                             -1, // NULL terminated
                             NULL, // address of wide-character buffer
                             0) + 1;               // size of buffer

  WCHAR * pWideChar = new WCHAR[size];

  MultiByteToWideChar(CP_UTF8, // code page
                      MB_ERR_INVALID_CHARS, // character-type options
                      utf8.c_str(), // address of string to map
                      -1, // NULL terminated
                      pWideChar, // address of wide-character buffer
                      size);                // size of buffer

  CLocaleString Locale(pWideChar);
  delete [] pWideChar;

  return Locale;
#endif // WIN32

#if (defined SunOS || defined Linux)
  static iconv_t Converter = NULL;

  if (Converter == NULL)
    {
      char From[] = "UTF-8";
      const char * To = findLocale();

      Converter = iconv_open(To, From);
    }

  if (Converter == (iconv_t)(-1))
    return CLocaleString(utf8.c_str());

  size_t Utf8Length = utf8.length();
  char * Utf8 = strdup(utf8.c_str());
#if (defined COPASI_ICONV_CONST_CHAR) // non standard iconv declaration :(
  const char * pUtf8 = Utf8;
#else
  char * pUtf8 = Utf8;
#endif

  size_t LocaleLength = Utf8Length + 1;
  size_t SpaceLeft = Utf8Length;
  char * Locale = new char[LocaleLength];
  char * pLocale = Locale;

  while (Utf8Length)
    if ((size_t)(-1) ==
        iconv(Converter, &pUtf8, &Utf8Length, &pLocale, &SpaceLeft))
      {
        switch (errno)
          {
            case EILSEQ:
              pUtf8 = Utf8;
              LocaleLength = 0;
              break;

            case EINVAL:
              pLocale = Locale;
              Utf8Length = 0;
              break;

            case E2BIG:
              char * pTmp = Locale;
              size_t OldLength = LocaleLength;
              LocaleLength += 2 * Utf8Length;

              Locale = new char[LocaleLength];
              memcpy(Locale, pTmp,
                     sizeof(char) * (OldLength - SpaceLeft - 1));
              pLocale = Locale + OldLength - SpaceLeft - 1;
              SpaceLeft += 2 * Utf8Length;
              delete [] pTmp;

              break;
          }

        continue;
      }

  *pLocale = 0x00; // NULL terminate the string.
  CLocaleString Result(Locale);

  // Reset the Converter
  iconv(Converter, NULL, &Utf8Length, NULL, &LocaleLength);

  // Release memory
  free(Utf8);
  delete [] Locale;

  return Result;
#endif // SunOS || Linux

#ifdef Darwin
  return CLocaleString(utf8.c_str());
#endif
}

CLocaleString::CLocaleString():
  mpStr(NULL)
{}

CLocaleString::CLocaleString(const CLocaleString::lchar * str):
  mpStr((str != NULL) ? strdup(str) : NULL)
{}

CLocaleString::CLocaleString(const CLocaleString & src):
  mpStr((src.mpStr != NULL) ? strdup(src.mpStr) : NULL)
{}

CLocaleString::~CLocaleString()
{
  if (mpStr != NULL)
    {
      free(mpStr);
      mpStr = NULL;
    }
}

CLocaleString & CLocaleString::operator = (const CLocaleString & rhs)
{
  if (mpStr != NULL)
    {
      free(mpStr);
      mpStr = NULL;
    }

  mpStr = (rhs.mpStr != NULL) ? strdup(rhs.mpStr) : NULL;

  return *this;
}

CLocaleString & CLocaleString::operator = (const CLocaleString::lchar * rhs)
{
  if (mpStr != NULL)
    {
      free(mpStr);
      mpStr = NULL;
    }

  mpStr = (rhs != NULL) ? strdup(rhs) : NULL;

  return *this;
}

std::string CLocaleString::toUtf8() const
{
  if (mpStr == NULL)
    {
      return "";
    }

#ifdef WIN32
  int size;

  size = WideCharToMultiByte(CP_UTF8, // code page
                             0, // performance and mapping flags
                             mpStr, // address of wide-character string
                             -1, // NULL terminated
                             NULL, // address of buffer for new string
                             0, // size of buffer
                             NULL, // address of default for unmappable characters
                             NULL) + 1; // address of flag set when default char used

  char * pUtf8 = new char[size];

  WideCharToMultiByte(CP_UTF8, // code page
                      0, // address of wide-character string
                      mpStr, // address of wide-character string
                      -1, // NULL terminated
                      pUtf8, // address of buffer for new string
                      size, // size of buffer
                      NULL, // address of default for unmappable characters
                      NULL);     // address of flag set when default char used

  std::string Utf8 = pUtf8;
  delete [] pUtf8;

  return Utf8;
#endif // WIN32

#if (defined SunOS || defined Linux)
  static iconv_t Converter = NULL;

  if (Converter == NULL)
    {
      char To[] = "UTF-8";
      const char * From = findLocale();

      Converter = iconv_open(To, From);
    }

  if (Converter == (iconv_t)(-1))
    return mpStr;

  size_t LocaleLength = strlen(mpStr);
  char * Locale = strdup(mpStr);
#if (COPASI_ICONV_CONST_CHAR) // non standard iconv declaration :(
  const char * pLocale = Locale;
#else
  char * pLocale = Locale;
#endif

  size_t Utf8Length = LocaleLength + 1;
  size_t SpaceLeft = LocaleLength;
  char * Utf8 = new char[Utf8Length];
  char * pUtf8 = Utf8;

  while (LocaleLength)
    if ((size_t)(-1) ==
        iconv(Converter, &pLocale, &LocaleLength, &pUtf8, &SpaceLeft))
      {
        switch (errno)
          {
            case EILSEQ:
              pUtf8 = Utf8;
              LocaleLength = 0;
              break;

            case EINVAL:
              pUtf8 = Utf8;
              LocaleLength = 0;
              break;

            case E2BIG:
              char * pTmp = Utf8;
              size_t OldLength = Utf8Length;
              Utf8Length += 2 * LocaleLength;

              Utf8 = new char[Utf8Length];
              memcpy(Utf8, pTmp,
                     sizeof(char) * (OldLength - SpaceLeft - 1));
              pUtf8 = Utf8 + OldLength - SpaceLeft - 1;
              SpaceLeft += 2 * LocaleLength;
              delete [] pTmp;

              break;
          }

        continue;
      }

  *pUtf8 = 0x00; // NULL terminate the string.
  std::string Result = Utf8;

  // Reset the Converter
  iconv(Converter, NULL, &LocaleLength, NULL, &Utf8Length);

  // Release memory
  free(Locale);
  delete [] Utf8;

  return Result;
#endif // SunOS || Linux

#ifdef Darwin
  std::string Result = mpStr;
  return Result;
#endif // Darwin
}

const CLocaleString::lchar * CLocaleString::c_str() const
{
  return mpStr;
}
