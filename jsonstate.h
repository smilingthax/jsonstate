#ifndef _JSONSTATE_H
#define _JSONSTATE_H

#include <vector>

#if defined(__GXX_EXPERIMENTAL_CXX0X__)||(__cplusplus>=201103L)
namespace detail {

template<int... S>
struct seq { };

} // namespace detail
#endif

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

  template<int State> void inState(int ch);
#if defined(__GXX_EXPERIMENTAL_CXX0X__)||(__cplusplus>=201103L)
  template<int... S> void callState(int ch,detail::seq<S...>);
#endif

  bool nextNumstate(char ch);
private:
  const char *err;

  std::vector<type_t> stack;
  enum state_t { sSTART, sSTRICT_START, sDONE, // _START or ARRAY_START or DICT(VALUE)_START
                 sVALUE_VERIFY, sVALUE_NUMBER,
                 sSTRING, sSTRING_ESCAPE, sSTRING_VALIDATE_ESCAPE, // VALUE_ or KEY_
                 sKEY_START, sKEY_UNQUOTED, sKEYDONE, // KEY_START actually has stack.push_back(KEY); delayed
                 sDICT_EMPTY, sARRAY_EMPTY,
                 sDICT_WAIT, sARRAY_WAIT,
                 _MAX_STATE_T=sARRAY_WAIT};
  state_t state;

  enum numstate_t {
    NSTART, NMINUS, NNZERO, NINT, NFRAC, NFRACMORE, // NZERO already used by some xopen headers
    NEXP, NEXPONE, NEXPMORE, NDONE
  };
  numstate_t numstate;

  int validate;
  const char *verify;
};

inline bool JsonState::inWait() const {
  return (!err)&&( (state==sDICT_WAIT)||(state==sARRAY_WAIT) );
}

inline bool JsonState::done() const {
  return (!err)&&(state==sDONE);
}
inline const char *JsonState::error() const {
  return err;
}

#endif
