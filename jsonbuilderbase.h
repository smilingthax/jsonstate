#ifndef _JSONBUILDERBASE_H
#define _JSONBUILDERBASE_H

#include <stdint.h>

// NOTE: does not enforce well-formedness

class JsonBuilderBase {
public:
  virtual ~JsonBuilderBase() {}

  virtual void startArray() {}
  virtual void startObject() {}
  virtual void endArray() {}
  virtual void endObject() {}

  virtual void stringKey(const char *start, const char *end) {} // obj
  virtual void stringValue(const char *start, const char *end) {} // which encoding?  -> utf8  (TODO? maybe another overload with wchar ?)
// TODO? numericValue(string)  only in JsonWriterBase ??  [but: too large number?]
  virtual void numericValue(const char *start, const char *end) {} // always ascii
  virtual void numericValue(double value) {}
  virtual void numericValue(int32_t value) {}  // TODO? int64_t
  virtual void boolValue(bool value) {}
  virtual void nullValue() {}
};

#endif
