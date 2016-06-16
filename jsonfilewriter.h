#ifndef _JSONFILEWRITER_H
#define _JSONFILEWRITER_H

#include "jsonwriter.h"
#include <stdio.h>

struct JsonBasicFileWriter : JBWriterBase {
  JsonBasicFileWriter(FILE *f=stdout) : f(f) {}

protected:
  void write(const char *str,size_t len) override {
    fwrite(str,len,1,stdout);
  }

private:
  FILE *f;
};

// not via template, because we want better control over ctor args (?... c++11 variadic template forwarding, when optional first arg is added?)
struct JsonPrettyFileWriter : JBPrettyWriter {
  JsonPrettyFileWriter(FILE *f=stdout,int indentAmount=2,char indentChar=' ')
    : JBPrettyWriter(indentAmount,indentChar),f(f)
  { }

protected:
  void write(const char *str,size_t len) override {
    fwrite(str,len,1,stdout);
  }

private:
  FILE *f;
};

#endif
