#ifndef _JSONPARSER_H
#define _JSONPARSER_H

#include "jsonbuilderbase.h"
#include "jsonstate.h"

struct JPListener;
class JsonParser {
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

private:
  JsonState state;

  friend struct JPListener;
//  std::unique_ptr<JPListener> jpl;  // FIXME c++11 only
  JPListener *jpl;
  const char *error;

  JsonParser(const JsonParser &);  // =delete;
  JsonParser &operator=(const JsonParser&); // =delete;
  // TODO? move?
};
#undef override

#endif
