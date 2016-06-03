#ifndef _JSONSTATE_H
#define _JSONSTATE_H

#include <vector>

class JsonState {
public:
  JsonState()
    : allowUnquotedKeys(false),
      validateEscapes(false),
      validateNumbers(false)
  {
    reset();
  }

  // Options, not affected by reset()
  bool allowUnquotedKeys;
  bool validateEscapes;
  bool validateNumbers;

  // Events
  bool Echar(int ch); // false on error
  bool Eend();   // needed to verify numbers and forbid empty string
  bool Ekey();   // optional shortcut for complete key
  bool Evalue(); // optional shortcut for complete value

  // States
//  bool inWait() const;    //  ','+More  vs.  '}' / ']'  expected next
  bool done() const;
  bool isFirst() const {  // true during first key(dict) / first value(array) / each value(dict)
    return first;
  }
  const char *error() const { // or NULL
    return err;
  }

  void reset(bool strictStart=false); // only Array/Dict, per RFC, if strict

  void dump() const;

  // Types
  enum type_t {
    ARRAY, OBJECT, // only these can be 'buried' in the stack (below the latest entry)
    VALUE_STRING, VALUE_NUMBER, VALUE_FALSE, VALUE_TRUE, VALUE_NULL,
    KEY_STRING,
    KEY_UNQUOTED  // extension
  };
  static const char *typeName(type_t type);

private:
  void gotStart(type_t type);
  type_t gotValue();

  bool nextNumstate(char ch);

private:
  const char *err;

  std::vector<type_t> stack;

  struct Trans;
  bool (*state)(JsonState &s,int ch);
  const char *stateDebug;

  bool first;

  enum numstate_t {    // storage size required... (c++11 enum class?)
    NSTART, NMINUS, NNZERO, NINT, NFRAC, NFRACMORE, // NZERO already used by some xopen headers
    NEXP, NEXPONE, NEXPMORE, NDONE
  };
  numstate_t numstate; // by Trans::VALUE_NUMBER

  int validate;        // by Trans::STRING_VALIDATE_ESCAPE
  const char *verify;  // by Trans::VALUE_VERIFY
};

#endif
