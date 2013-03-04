#include "jsonstate.h"
#include <assert.h>
#include <stdio.h>

#if !defined(__GXX_EXPERIMENTAL_CXX0X__)&&(__cplusplus<201103L)
  #define nullptr NULL
  // can't use JsonState::state_t::state in C++03
  #define STATE_T JsonState
#else
  #define CPP11
  #define STATE_T JsonState::state_t
#endif

#ifdef CPP11
namespace detail { // helper to build index list

template<int N, int... S>
struct gens : gens<N-1, N-1, S...> { };

template<int... S>
struct gens<0, S...> {
  typedef seq<S...> type;
};

} // namespace detail
#endif

JsonState::JsonState()
  : allowUnquotedKeys(false),
    validateEscapes(false),
    validateNumbers(false),
    err(nullptr),state(START)
{
}

void JsonState::reset(bool strictStart) // {{{
{
  err=nullptr;
  stack.clear();
  if (strictStart) {
    state=STRICT_START;
  } else {
    state=START;
  }
}
// }}}

bool JsonState::is_ws(char c) const // {{{
{
  return ( (c==' ')||(c=='\r')||(c=='\n')||(c=='\t') );
}
// }}}

bool JsonState::is_escape(char c) const // {{{
{
  return (c=='\\')||(c=='"')||(c=='\\')||(c=='/')||
         (c=='b')||(c=='f')||(c=='n')||(c=='r')||(c=='t')||(c=='u');
}
// }}}

bool JsonState::is_digit(char c) const // {{{
{
  return ( (c>='0')&&(c<='9') );
}
// }}}

bool JsonState::is_hex(char c) const // {{{
{
  return ( (c>='0')&&(c<='9') )||
         ( (c>='a')&&(c<='f') )||
         ( (c>='A')&&(c<='F') );
}
// }}}

#define TF(state,code) template<> \
              void JsonState::inState<STATE_T::state>(char ch) code

#define TSkipBool(state,skipcond,cond,code,elsecode) \
              TF(state,{ if (skipcond) return; \
                         if cond code else elsecode })
#define TBool(state,cond,code,elsecode) \
              TF(state,{ if cond code else elsecode })

#define TSkipSwitch(state,skipcond) \
              TF(state,{ if (skipcond) return; \
                         switch (ch) {)
#define TSwitch(state) \
              TF(state,{ switch (ch) {)
#define TCase(char,code) case char: code; break;
#define TEndSwitch(code) default: code; break; } }


TSkipSwitch(START, (is_ws(ch)))
TCase('{', { stack.push_back(DICT); state=DICT_EMPTY; })
TCase('[', { stack.push_back(ARRAY); state=ARRAY_EMPTY; })
TCase('"', { stack.push_back(VALUE); state=STRING; })
TCase('f', { stack.push_back(VALUE); state=VALUE_VERIFY; verify="alse"; })
TCase('t', { stack.push_back(VALUE); state=VALUE_VERIFY; verify="rue"; })
TCase('n', { stack.push_back(VALUE); state=VALUE_VERIFY; verify="ull"; })
TEndSwitch({
  if ( (ch=='-')||(is_digit(ch)) ) {
    stack.push_back(VALUE);
    state=VALUE_NUMBER;
    if (validateNumbers) {
      numstate=NSTART;
      if (!nextNumstate(ch)) {
        err="Invalid Number";
      }
    }
  } else {
    err="Unexpected character"; // TODO output ch?
  }
})

TSkipSwitch(STRICT_START, (is_ws(ch)))
TCase('{', { stack.push_back(DICT); state=DICT_EMPTY; })
TCase('[', { stack.push_back(ARRAY); state=ARRAY_EMPTY; })
TEndSwitch({
  err="Array or Object expected";
})

TBool(VALUE_VERIFY, (ch==*verify), {
  verify++;
  if (!*verify) {
    gotValue();
  }
},{ err="Expected true/false/null keyword"; })

TBool(VALUE_NUMBER, ( (is_digit(ch))||(ch=='.')||(ch=='+')||(ch=='-')||(ch=='e')||(ch=='E') ), {
  if ( (validateNumbers)&&(!nextNumstate(ch)) ) {
    err="Invalid Number";
  }
},{
  if ( (validateNumbers)&&(numstate!=NDONE)&&(!nextNumstate(0)) ) {
    err="Premature end of number";
  } else {
    gotValue(); // DONE, DICT_WAIT or ARRAY_WAIT now
    Echar(ch);  // allowed next: is_ws or ",}]"
  }
})

TSwitch(STRING)
TCase('\\',{ state=STRING_ESCAPE; })
TCase('"', { gotValue(); })
TEndSwitch({ }) // TODO? check for valid chars?

TF(STRING_ESCAPE, {
  state=STRING;
  if (validateEscapes) {
    if (ch=='u') {
      state=STRING_VALIDATE_ESCAPE;
      validate=4;
    } else if (!is_escape(ch)) {
      err="Invalid escape";
    }
  }
});

TBool(STRING_VALIDATE_ESCAPE, (is_hex(ch)), {
  validate--;
  if (!validate) {
    state=STRING;
  }
},{ err="Invalid unicode escape"; })

TSkipBool(KEY_START, (is_ws(ch)), (ch=='"'), {
  stack.push_back(KEY);
  state=STRING;
} else if (allowUnquotedKeys) {
  stack.push_back(KEY);
  state=KEY_UNQUOTED;
},{
  err="Expected dict key";
})

TBool(KEY_UNQUOTED, ( (is_ws(ch))||(ch==':') ), {
  // TODO? synthesize '"'
  gotValue();
},{
  if (ch=='\\') {
    // BAD... TODO? synthesize another '\\'
    err="Bad unquoted key";
  }
})

TSkipBool(KEYDONE, (is_ws(ch)), (ch==':'), {
  state=START; // DICTVALUE_START
},{
  err="Expected ':' after key";
})

TSkipBool(DICT_EMPTY, (is_ws(ch)), (ch=='}'), {
  gotValue();
},{ inState<KEY_START>(ch); }) // epsilon transition

TSkipBool(ARRAY_EMPTY, (is_ws(ch)), (ch==']'), {
  gotValue();
},{ inState<START>(ch); }) // epsilon transition

TSkipSwitch(DICT_WAIT, (is_ws(ch)))
TCase(',', { state=KEY_START; })
TCase('}', { gotValue(); })
TEndSwitch({ err="Expected ',' or '}'"; }) // "instead of [ch]"

TSkipSwitch(ARRAY_WAIT, (is_ws(ch)))
TCase(',', { state=START; })
TCase(']', { gotValue(); })
TEndSwitch({ err="Expected ',' or ']'"; })

TBool(DONE, (!is_ws(ch)), {
  err="Trailing character";
},{ })

void JsonState::gotValue() // {{{
{
  assert( (!err)&&(!stack.empty()) );
  const mode_t oldVal=stack.back();
  stack.pop_back();
  if (stack.empty()) {
    state=DONE;
  } else if (oldVal==KEY) {
    state=KEYDONE;
  } else {
    const mode_t val=stack.back();
    if (val==DICT) {
      state=DICT_WAIT;
    } else if (val==ARRAY) {
      state=ARRAY_WAIT;
    } else {
      assert(0);
      err="Internal error";
    }
  }
}
// }}}

// CPP11 trick: http://stackoverflow.com/questions/7381805/c-c11-switch-statement-for-variadic-templates
#ifdef CPP11
template<int... S>
void JsonState::callState(char ch,detail::seq<S...>)
{
  static constexpr decltype(&JsonState::inState<0>) table[]={
//  static constexpr void (JsonState::*table[])(char)={
    &JsonState::inState<S>...
  };
  (this->*table[state])(ch);
}
#endif

bool JsonState::Echar(char ch) // {{{
{
  if (err) {
    return false;
  }

#ifdef CPP11
  callState(ch,detail::gens<state_t::_MAX_STATE_T+1>::type());
#else
  switch (state) { // compiler(g++) warns, if the cases do not match the enum
  case 0: inState<0>(ch); break;
  case 1: inState<1>(ch); break;
  case 2: inState<2>(ch); break;
  case 3: inState<3>(ch); break;
  case 4: inState<4>(ch); break;
  case 5: inState<5>(ch); break;
  case 6: inState<6>(ch); break;
  case 7: inState<7>(ch); break;
  case 8: inState<8>(ch); break;
  case 9: inState<9>(ch); break;
  case 10: inState<10>(ch); break;
  case 11: inState<11>(ch); break;
  case 12: inState<12>(ch); break;
  case 13: inState<13>(ch); break;
  case 14: inState<14>(ch); break;
  }
#endif

  return (!err);
}
// }}}

bool JsonState::Eend() // {{{
{
  if ( (Echar(' '))&&(state!=DONE) ) {
    err="Premature end of input";
  }
  return (!err);
}
// }}}

bool JsonState::Ekey() // {{{
{
  if (!err) {
    if ( (state==KEY_START)||(state==DICT_EMPTY) ) {
      state=KEYDONE;
    } else {
      err="Unexpected key";
    }
  }
  return (!err);
}
// }}}

bool JsonState::Evalue() // {{{
{
  if ( (state==KEY_START)||(state==DICT_EMPTY) ) {
    return Ekey();
  }
  if (!err) {
    if ( (state==START)||(state==ARRAY_EMPTY) ) {
      stack.push_back(VALUE);
      gotValue();
    } else {
      err="Unexpected value";
    }
  }
  return (!err);
}
// }}}

bool JsonState::nextNumstate(char ch) // {{{
{
  switch (numstate) {
  case NSTART:
    if (ch=='-')        { numstate=NMINUS; break; }
  case NMINUS:
    if (ch=='0')        { numstate=NNZERO; break; }
    if (is_digit(ch))   { numstate=NINT; break; }
    return false; // TODO? numstate=NERROR; break/continue;
  case NINT:
    if (is_digit(ch))   { break; }
  case NNZERO:
    if (ch=='.')        { numstate=NFRAC; break; }
  // NLOOKEXP:
    if ((ch|0x20)=='e') { numstate=NEXP; break; }
    if (!ch)            { numstate=NDONE; break; }
    return false;
  case NFRAC:
    if (is_digit(ch))   { numstate=NFRACMORE; break; }
    return false;
  case NFRACMORE:
    if (is_digit(ch))   { break; }
  // NLOOKEXP:
    if ((ch|0x20)=='e') { numstate=NEXP; break; }
    if (!ch)            { numstate=NDONE; break; }
    return false;
  case NEXP:
    if ( (ch=='+')||(ch=='-') ) { numstate=NEXPONE; break; }
    if (is_digit(ch))   { numstate=NEXPMORE; break; }
    return false;
  case NEXPONE:
    if (is_digit(ch))   { numstate=NEXPMORE; break; }
    return false;
  case NEXPMORE:
    if (is_digit(ch))   { break; }
    if (!ch)            { numstate=NDONE; break; }
    return false;
  case NDONE:
    return false;
  }
  return true;
}
// }}}

void JsonState::dump() const // {{{
{
  static const char *mode2str[]={"DICT","ARRAY","KEY","VALUE"};
  // TODO? map state to string
  if (err) {
    printf("Error: %s (State: %d)\n",err,state);
  } else if (state==DONE) {
    puts("Done");
  } else {
    printf("State: %d, Stack: [",state);
    for (size_t iA=0;iA<stack.size();iA++) {
      if (iA) {
        putchar(',');
      }
      printf("%s",mode2str[stack[iA]]);
    }
    if ( (state==VALUE_NUMBER)&&(validateNumbers) ) {
      printf("], Numstate: %d\n",numstate);
    } else {
      puts("]");
    }
  }
}
// }}}

