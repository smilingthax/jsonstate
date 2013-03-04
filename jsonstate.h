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
  bool Echar(char ch); // false on error
  bool Eend();   // needed to verify numbers and forbid empty string
  bool Ekey();   // optional shortcut for complete key
  bool Evalue(); // optional shortcut for complete value

  // States
  bool inWait() const;    //  either ','+Value or '}' / ']' next

  bool done() const;
  const char *error() const; // or NULL

  void reset(bool strictStart=false); // only Array/Dict, per RFC, if strict

  // Helper
  bool is_ws(char ch) const;
  bool is_escape(char ch) const;
  bool is_digit(char ch) const;
  bool is_hex(char ch) const;

  void dump() const;
private:
  void gotValue();

  template<int State> void inState(char ch);
#if defined(__GXX_EXPERIMENTAL_CXX0X__)||(__cplusplus>=201103L)
  template<int... S> void callState(char ch,detail::seq<S...>);
#endif

  bool nextNumstate(char ch);
private:
  const char *err;

  enum mode_t {
    DICT,
    ARRAY,
    // only possible at the top:
    KEY,
    VALUE
  };
  std::vector<mode_t> stack;
  enum state_t { START, STRICT_START, DONE, // _START or ARRAY_START or DICT(VALUE)_START
                 VALUE_VERIFY, VALUE_NUMBER,
                 STRING, STRING_ESCAPE, STRING_VALIDATE_ESCAPE, // VALUE_ or KEY_
                 KEY_START, KEY_UNQUOTED, KEYDONE, // KEY_START actually has stack.push_back(KEY); delayed
                 DICT_EMPTY, ARRAY_EMPTY,
                 DICT_WAIT, ARRAY_WAIT,
                 _MAX_STATE_T=ARRAY_WAIT};
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
  return (!err)&&( (state==DICT_WAIT)||(state==ARRAY_WAIT) );
}

inline bool JsonState::done() const {
  return (!err)&&(state==DONE);
}
inline const char *JsonState::error() const {
  return err;
}

#endif
