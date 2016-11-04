#include "jsonparser.h"
#include <assert.h>

#if !defined(__GXX_EXPERIMENTAL_CXX0X__)&&(__cplusplus<201103L)
  #define nullptr NULL
  #define override
#endif

#include <double-conversion/double-conversion.h>
#include <cmath>

// assumes validated input (no junk, not empty -> otherwise: NaN)
static double parseDouble(const char *start, const char *end) // {{{
{
  using namespace double_conversion;

  static StringToDoubleConverter converter(StringToDoubleConverter::NO_FLAGS,
                                           NAN, // empty string already excluded
                                           NAN, // no junk
                                           NULL, // no infinity symbol
                                           NULL); // no NaN symbol

  int consumed;
  return converter.StringToDouble(start,end-start,&consumed);
  // because flags don't allow trailing garbage: consumed==end-start
}
// }}}

#include <string>

// TODO? ... unquoted_key currently mostly ignored wrt. output
static void stringKey(JsonBuilderBase &dst,const std::string &str,bool unquoted_key=false) // {{{
{
  const char *cstr=str.c_str();
  if (unquoted_key) {
    dst.stringKey(cstr,cstr+str.size());
  } else { // remove quotes
    assert(str.size()>=2); // TODO?!
    dst.stringKey(cstr+1,cstr+str.size()-1);
  }
}
// }}}

static void stringValue(JsonBuilderBase &dst,const std::string &str) // {{{
{
  const char *cstr=str.c_str();
  // remove quotes
  assert(str.size()>=2); // TODO?!
  dst.stringValue(cstr+1,cstr+str.size()-1);
}
// }}}

#include <limits>

static void parseNumber(JsonBuilderBase &dst,const std::string &str,bool numbersTryInt=false) // {{{
{
  const char *cstr=str.c_str();
  double value=parseDouble(cstr,cstr+str.size());
  assert(!std::isnan(value)); // (input is validated)

  if (numbersTryInt) {
    typedef std::numeric_limits<int32_t> limit;
    if ( (value>=limit::min())&&(value<=limit::max()) ) {
      double dummy;
      if (std::modf(value,&dummy)==0.0) {
        dst.numericValue((int32_t)value);
        return;
      }
    }
  }
  dst.numericValue(value);
}
// }}}

#include "jsunescape.tcc"
#include "utf/utf8.tcc"

// TODO... template<Output> ?
struct JPListener : JsonState::EventListenerBase {
  JPListener(JsonParser &parent,JsonBuilderBase &dst) : parent(parent),dst(dst),limit(0),utfwriter(appender),escaper(utfwriter) {}

  const char *startValue(JsonState &state,JsonState::type_t type) override;
  void endValue(JsonState &state,JsonState::type_t type) override;

  void next(int ch);
protected:
  void beginCollect(int limit,bool decodeEscapes);
  const std::string &endCollect();

private:
  JsonParser &parent;
  JsonBuilderBase &dst;

  struct Appender {
    void operator()(const char *start,const char *end) {
      collect.append(start,end);
    }

    std::string collect;
  };

  int limit;
  Appender appender;
  UtfFsm::Utf8Writer<Appender&> utfwriter;
  JsUnescapeFSM<UtfFsm::Utf8Writer<Appender&>&> escaper;
};

void JPListener::beginCollect(int _limit,bool decodeEscapes) // {{{
{
  assert(_limit!=0); // 0: internal - "do not collect"
  appender.collect.clear();
  limit=_limit;
  escaper.reset(decodeEscapes);
}
// }}}

void JPListener::next(int ch) // {{{
{
  if (limit!=0) { // collecting
    if ( (appender.collect.size()>=(unsigned int)limit) ) {
      parent.error="Limit reached";
      limit=0; // stop collecting ...
      return;
    }
    assert( (ch>=0)&&(ch<0x110000) );
    if (!escaper(ch)) {
      parent.error="Bad escape sequence";
    }
  }
}
// }}}


// NOTE: retval will not stay around!
const std::string &JPListener::endCollect() // {{{
{
  if (!escaper(UtfFsm::EndOfInput)) {
    assert(false); // could only fail when unquoted (but these have decodeEscapes==false)
  }
  limit=0;
  return appender.collect;
}
// }}}

const char *JPListener::startValue(JsonState &state,JsonState::type_t type) // {{{
{
  switch (type) {
  case JsonState::ARRAY:
    dst.startArray();
    break;
  case JsonState::OBJECT:
    dst.startObject();
    break;
  case JsonState::KEY_UNQUOTED:
    beginCollect(parent.keyLimit-2,false);
    break;
  case JsonState::KEY_STRING:
    beginCollect(parent.keyLimit,true);
    break;
  case JsonState::VALUE_STRING:
    beginCollect(parent.stringLimit,true);
    break;
  case JsonState::VALUE_NUMBER:
    beginCollect(parent.numberLimit,false);
    break;
  case JsonState::VALUE_FALSE:
  case JsonState::VALUE_TRUE:
  case JsonState::VALUE_NULL:
    break;
  }
  return nullptr;
}
// }}}

void JPListener::endValue(JsonState &state,JsonState::type_t type) // {{{
{
  switch (type) {
  case JsonState::ARRAY:
    dst.endArray();
    break;
  case JsonState::OBJECT:
    dst.endObject();
    break;
  case JsonState::KEY_UNQUOTED:
    stringKey(dst,endCollect(),true);
    break;
  case JsonState::KEY_STRING:
    stringKey(dst,endCollect());
    break;
  case JsonState::VALUE_STRING:
    stringValue(dst,endCollect());
    break;
  case JsonState::VALUE_NUMBER:
    parseNumber(dst,endCollect(),parent.numbersTryInt);
    break;
  case JsonState::VALUE_FALSE:
    dst.boolValue(false);
    break;
  case JsonState::VALUE_TRUE:
    dst.boolValue(true);
    break;
  case JsonState::VALUE_NULL:
    dst.nullValue();
    break;
  }
}
// }}}


JsonParser::JsonParser(JsonBuilderBase &builder)
  : numbersTryInt(false),
    keyLimit(256),
    stringLimit(-1),
    numberLimit(20),
    error(nullptr)
{
//  state.reset(true);  // FIXME... config from where?
  state.validateNumbers=true; // parseNumber/parseDouble   depends on it!
//state.validateEscapes=true;
//state.allowUnquotedKeys=true;

  jpl=new JPListener(*this,builder);
  state.setListener(jpl);
}

JsonParser::~JsonParser()
{
  delete jpl;
}

void JsonParser::reset()
{
  error=nullptr;
  state.reset(); // (false) / (true) ...
}

void JsonParser::reset(JsonBuilderBase &newBuilder)
{
  delete jpl;
  jpl=new JPListener(*this,newBuilder);
  state.setListener(jpl);
  reset();
}

// TODO? swap out  this->next  ptr  (but: compiler already does static calls from here)
bool JsonParser::next(int ch)
{
  if (!state.Echar(ch)) {
    return false;
  }
  jpl->next(ch);
  return (!error);
}

const char *JsonParser::end() // {{{
{
  if (error) {
    return error;
  }
  if (!state.Eend()) {
    return state.error();
  }
  return nullptr;
}
// }}}

