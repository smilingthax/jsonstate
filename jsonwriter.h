#ifndef _JSONWRITER_H
#define _JSONWRITER_H

#include "jsonbuilderbase.h"
#include <stddef.h>

#if !defined(__GXX_EXPERIMENTAL_CXX0X__)&&(__cplusplus<201103L)
  #define override
#endif

class JBWriterBase : public JsonBuilderBase {
public:
  JBWriterBase() : first(true) {}

  void startArray() override;
  void startObject() override;
  void endArray() override;
  void endObject() override;

  void stringKey(const char *start, const char *end) override;
  void stringValue(const char *start, const char *end) override;
  void numericValue(const char *start, const char *end) override;
  void numericValue(double value) override; // forwards after stringify
  void numericValue(int32_t value) override; // forwards to (start,end)
  void boolValue(bool value) override;
  void nullValue() override;
protected:
  virtual void write(const char *str,size_t len) =0;
  virtual void addSeparator();

  struct Writer; // in c++11 this would be a lambda / std::bind

  bool first; // used by addSeparator
};

#include <string>

class JBPrettyWriter : public JBWriterBase {
public:
  JBPrettyWriter(int indentAmount=2,char indentChar=' '); // or '\t'

  void startArray() override;
  void startObject() override;
  void endArray() override;
  void endObject() override;

  void stringKey(const char *start, const char *end) override;
private:
  void pushIndent();
  void popIndent();
  void addSeparator() override;
private:
  char indentChar;
  int indentAmount;

  std::string indent;
  bool isKey;
  bool veryFirst;
};

#undef override

#endif
