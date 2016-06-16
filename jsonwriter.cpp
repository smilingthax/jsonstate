#include "jsonwriter.h"
#include <assert.h>
#include <stdexcept>

#if defined(__GXX_EXPERIMENTAL_CXX0X__)||(__cplusplus>=201103L)
  #define CPP11
  #include <string>
#else
  #include <stdio.h>
#endif

#include "jsonchars.tcc"  // WriterBase: escaping(is_control), prettyWriter(is_ws)

void JBWriterBase::startArray() // {{{
{
  write("[",1);
}
// }}}

void JBWriterBase::startObject() // {{{
{
  write("{",1);
}
// }}}

void JBWriterBase::endArray() // {{{
{
  write("]",1);
}
// }}}

void JBWriterBase::endObject() // {{{
{
  write("}",1);
}
// }}}

template <typename Output> // void out(start,end)
struct JsonSegmenter { // {{{      FIXME
  JsonSegmenter(Output out) : out(out) {}

  bool mustEscape(char ch) {
    return (JsonChars::is_control0(ch))|| // superset of \b\f\n\r\t
//           (ch=='/')||  // optional!
           (ch=='"')||(ch=='\\');
  }
  void writeEscape(char ch) {
    static const char hex[]="0123456789abcdef";
    char shortch=shortEscape(ch);
    if (shortch) {
      char shortesc[2]={'\\',shortch};
      out(shortesc,shortesc+2);
    } else {
      char longesc[6]={'\\','u','0','0',hex[(ch>>4)&0x0f],hex[ch&0x0f]};
      out(longesc,longesc+6);
    }
  }
  void write(const char *start,const char *end) {
    out(start,end);
  }
  void writeUtf8(int ch,const char *start,const char *end) {
    out(start,end); // just take original encoding
  }
  bool error(const char *start,const char *end) {
    return true; // TODO?
  }

private:
  Output out;
  // ... reverse of JsUnescapeFSM::shortCodes  // ... optional: '/'
  static char shortEscape(char ch) { // {{{
    switch (ch) {
    case '"': return '"';
    case '\b': return 'b';
    case '\f': return 'f';
    case '\n': return 'n';
    case '\r': return 'r';
    case '\t': return 't';
    case '/': return '/'; // optional
    case '\\': return '\\';
    default: return 0;
    }
  }
  // }}}
};
// }}}

#include "utf/utf8segmenter.tcc"

struct JBWriterBase::Writer {
  Writer(JBWriterBase &parent) : parent(parent) {} // implicit
  void operator()(const char *start,const char *end) {
    parent.write(start,end-start);
  }
  JBWriterBase &parent;
};

void JBWriterBase::stringValue(const char *start, const char *end) // {{{
{
  write("\"",1);
  if (!utf8segmenter(JsonSegmenter<Writer>(*this),start,end)) {
    assert(false); // TODO ...
  }
  write("\"",1);
}
// }}}

// NOTE: unchecked
void JBWriterBase::numericValue(const char *start, const char *end) // {{{
{
  write(start,end-start);
}
// }}}

// good enough
void JBWriterBase::numericValue(int32_t value) // {{{
{
#ifdef CPP11
  std::string str=std::to_string(value);
  numericValue(str.data(),str.data()+str.size());
#else
  char buf[32];
  const int len=snprintf(buf,32,"%d",value);
  assert(len<32);
  numericValue(buf,buf+len);
#endif
}
// }}}

#include <double-conversion/double-conversion.h>

void JBWriterBase::numericValue(double value) // {{{
{
  using namespace double_conversion;
  // similar to EcmaScriptConverter
  static DoubleToStringConverter converter(DoubleToStringConverter::NO_FLAGS, // UNIQUE_ZERO? EMIT_TRAILING_? EMIT_POSITIVE_?
                                           "1e999", // "Infinity" not allowed in JSON
                                           NULL,    // "NaN", not allowed
                                           'e',
                                           -6, 21,
                                           6, 0);

  char buf[DoubleToStringConverter::kBase10MaximalLength+8]; // don't forget sign, point, 'e', expsign, 3 expdigits, \0

  StringBuilder build(buf,sizeof(buf));
  if (!converter.ToShortest(value,&build)) {
    throw std::invalid_argument("Can't output NaN in JSON");
  }
  numericValue(buf,buf+build.position());
// or: char *res=build.Finalize();
//  numericValue(res,res+strlen(res));
}
// }}}

void JBWriterBase::boolValue(bool value) // {{{
{
  if (value) {
    write("true",4);
  } else {
    write("false",5);
  }
}
// }}}

void JBWriterBase::nullValue() // {{{
{
  write("null",4);
}
// }}}

void JBWriterBase::separator(bool key) // {{{
{
  if (key) {
    write(":",1);
  } else {
    write(",",1);
  }
}
// }}}


JBPrettyWriter::JBPrettyWriter(int indentAmount,char indentChar)
  : indentChar(indentChar),
    indentAmount(indentAmount),
    indent(1,'\n'),
    skipIndent(false),
    veryFirst(true)
{
  assert(JsonChars::is_ws(indentChar));
}

void JBPrettyWriter::startArray() // {{{
{
  writeIndent();
  JBWriterBase::startArray();
  pushIndent();
}
// }}}

void JBPrettyWriter::startObject() // {{{
{
  writeIndent();
  JBWriterBase::startObject();
  pushIndent();
}
// }}}

void JBPrettyWriter::endArray() // {{{
{
  popIndent();
  writeIndent();
  JBWriterBase::endArray();
}
// }}}

void JBPrettyWriter::endObject() // {{{
{
  popIndent();
  writeIndent();
  JBWriterBase::endObject();
}
// }}}

void JBPrettyWriter::stringValue(const char *start, const char *end) // {{{
{
  writeIndent();
  JBWriterBase::stringValue(start,end);
}
// }}}

void JBPrettyWriter::numericValue(const char *start, const char *end) // {{{
{
  writeIndent();
  JBWriterBase::numericValue(start,end);
}
// }}}

void JBPrettyWriter::boolValue(bool value) // {{{
{
  writeIndent();
  JBWriterBase::boolValue(value);
}
// }}}

void JBPrettyWriter::nullValue() // {{{
{
  writeIndent();
  JBWriterBase::nullValue();
}
// }}}

void JBPrettyWriter::separator(bool key) // {{{
{
  if (key) {
    write(": ",2);
    skipIndent=true;
  } else {
    JBWriterBase::separator(false);
    // writeIndent(); skipIndent=true;  ?
  }
}
// }}}


inline void JBPrettyWriter::pushIndent() // {{{
{
  indent.append(indentAmount,indentChar);
}
// }}}

inline void JBPrettyWriter::popIndent() // {{{
{
  assert(indent.size()-indentAmount>=1);
  indent.resize(indent.size()-indentAmount);
}
// }}}

inline void JBPrettyWriter::writeIndent() // {{{
{
  if (skipIndent) {
    skipIndent=false;
    // veryFirst=false; ?
  } else if (veryFirst) {
    write(indent.data()+1,indent.size()-1); // without '\n'
    veryFirst=false;
  } else {
    write(indent.data(),indent.size());
  }
}
// }}}

