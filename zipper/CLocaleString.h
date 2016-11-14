// Copyright (C) 2011 - 2010 by Pedro Mendes, Virginia Tech Intellectual
// Properties, Inc., University of Heidelberg, and The University
// of Manchester.
// All rights reserved.

#include <string>

#ifndef ZIPPER_CLocaleString
#define ZIPPER_CLocaleString

namespace zipper
{

class CLocaleString
{
public:
#ifdef WIN32
  typedef wchar_t lchar;
#else
  typedef char lchar;
#endif

  static CLocaleString fromUtf8(const std::string & utf8);

  // Operations
  CLocaleString();

  CLocaleString(const lchar * str);

  CLocaleString(const CLocaleString & src);

  ~CLocaleString();

  CLocaleString & operator = (const CLocaleString & rhs);

  CLocaleString & operator = (const lchar * str);

  std::string toUtf8() const;

  const lchar * c_str() const;

private:
  // Attributes
  lchar * mpStr;
};
}
#endif // ZIPPER_CLocaleString
