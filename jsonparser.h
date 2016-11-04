#ifndef _JSONPARSER_H
#define _JSONPARSER_H

#include "jsonbuilderbase.h"
#include "jsonstate.h"
#include "utf/utffsm.h"

struct JPListener;
class JsonParser { // push-type interface
public:
  JsonParser(JsonBuilderBase &builder);
  ~JsonParser();

  // NOTE: JBWriterBase uses DoubleToStringConverter::ToShortest ... (which also kills fractional parts...)
  bool numbersTryInt;

  int keyLimit;
  int stringLimit;
  int numberLimit;

  // will not affect options, above
  void reset();
  void reset(JsonBuilderBase &newBuilder);

// TODO? "will not call numericValue(string)"   // or: options: alwaysParseAsDouble, tryParseAsDouble, numbersAsString ...?

  bool next(int ch);
  const char *end(); // error or nullptr

  // FSM-type interface
  bool operator()(int ch) {
    if (ch==UtfFsm::EndOfInput) { // end of input
      return (end()==0); // nullptr
    }
    return next(ch);
  }
  const char *error() {
    if (err) {
      return err;
    }
    return state.error();
  }

private:
  JsonState state;

  friend struct JPListener;
//  std::unique_ptr<JPListener> jpl;  // FIXME c++11 only
  JPListener *jpl;
  const char *err;

  JsonParser(const JsonParser &);  // =delete;
  JsonParser &operator=(const JsonParser&); // =delete;
  // TODO? move?
};
#undef override

#endif
