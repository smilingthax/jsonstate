#ifndef _JSONSTATE_H
#define _JSONSTATE_H

#include <vector>

class JsonState {
public:
  JsonState();

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
  bool inWait() const;    //  either ','+Value or '}' / ']' expected next

  bool done() const;
  const char *error() const; // or NULL

  void reset(bool strictStart=false); // only Array/Dict, per RFC, if strict

  void dump() const;

  // Types
  enum type_t {
    ARRAY, OBJECT, // only these can be 'buried' in the stack (below the latest entry)
    VALUE_STRING, VALUE_NUMBER, VALUE_BOOL, VALUE_NULL,
    KEY_STRING,
    KEY_UNQUOTED  // extension
  };

private:
  void gotStart(type_t type);
  void gotValue();

  // "Modes"
  bool isKey(type_t type) const { return (type==KEY_STRING)||(type==KEY_UNQUOTED); }

  bool nextNumstate(char ch);

private:
  const char *err;

  std::vector<type_t> stack;
  void sSTART(int ch); // _START or ARRAY_START or DICT(VALUE)_START
  void sSTRICT_START(int ch);
  void sDONE(int ch);

  void sVALUE_VERIFY(int ch);
  void sVALUE_NUMBER(int ch);

  void sSTRING(int ch); // VALUE_ or KEY_
  void sSTRING_ESCAPE(int ch);
  void sSTRING_VALIDATE_ESCAPE(int ch);

  void sKEY_START(int ch); // KEY_START actually contains the (delayed)  gotStart(KEY_STRING)
  void sKEY_UNQUOTED(int ch);
  void sKEYDONE(int ch);

  void sDICT_EMPTY(int ch);
  void sARRAY_EMPTY(int ch);

  void sDICT_WAIT(int ch);
  void sARRAY_WAIT(int ch);

  void (JsonState::*state)(int ch); // points to some &JsonState::sNAME
  const char *stateDebug;

  enum numstate_t {
    NSTART, NMINUS, NNZERO, NINT, NFRAC, NFRACMORE, // NZERO already used by some xopen headers
    NEXP, NEXPONE, NEXPMORE, NDONE
  };
  numstate_t numstate;

  int validate;
  const char *verify;
};

inline bool JsonState::inWait() const {
  return (!err)&&( (state==&JsonState::sDICT_WAIT)||(state==&JsonState::sARRAY_WAIT) );
}

inline bool JsonState::done() const {
  return (!err)&&(state==&JsonState::sDONE);
}
inline const char *JsonState::error() const {
  return err;
}

#endif
