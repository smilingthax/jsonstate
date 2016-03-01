#include "jsonstate.h"
#include <assert.h>
#include <stdio.h>

#include "jsonchars.tcc"

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
    err(nullptr),state(sSTART)
{
}

void JsonState::reset(bool strictStart) // {{{
{
  err=nullptr;
  stack.clear();
  if (strictStart) {
    state=sSTRICT_START;
  } else {
    state=sSTART;
  }
}
// }}}


#define TF(state,code) template<> \
              void JsonState::inState<STATE_T::s ## state>(int ch) code

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
#define TCase(c,code) case c: code; break;
#define TEndSwitch(code) default: code; break; } }


TSkipSwitch(START, (JsonChars::is_ws(ch)))
TCase('{', { gotStart(OBJECT);       state=sDICT_EMPTY; })
TCase('[', { gotStart(ARRAY);        state=sARRAY_EMPTY; })
TCase('"', { gotStart(VALUE_STRING); state=sSTRING; })
TCase('f', { gotStart(VALUE_BOOL);   state=sVALUE_VERIFY; verify="alse"; })
TCase('t', { gotStart(VALUE_BOOL);   state=sVALUE_VERIFY; verify="rue"; })
TCase('n', { gotStart(VALUE_NULL);   state=sVALUE_VERIFY; verify="ull"; })
TEndSwitch({
  if ( (ch=='-')||(JsonChars::is_digit(ch)) ) {
    gotStart(VALUE_NUMBER);
    state=sVALUE_NUMBER;
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

TSkipSwitch(STRICT_START, (JsonChars::is_ws(ch)))
TCase('{', { gotStart(OBJECT); state=sDICT_EMPTY; })
TCase('[', { gotStart(ARRAY); state=sARRAY_EMPTY; })
TEndSwitch({
  err="Array or Object expected";
})

TBool(VALUE_VERIFY, (ch==*verify), {
  verify++;
  if (!*verify) {
    gotValue();
  }
},{ err="Expected true/false/null keyword"; })

TBool(VALUE_NUMBER, ( (JsonChars::is_digit(ch))||(ch=='.')||(ch=='+')||(ch=='-')||(ch=='e')||(ch=='E') ), {
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
TCase('\\',{ state=sSTRING_ESCAPE; })
TCase('"', { gotValue(); })
TEndSwitch({ }) // TODO? check for valid chars?

TF(STRING_ESCAPE, {
  state=sSTRING;
  if (validateEscapes) {
    if (ch=='u') {
      state=sSTRING_VALIDATE_ESCAPE;
      validate=4;
    } else if (!JsonChars::is_escape(ch)) {
      err="Invalid escape";
    }
  }
});

TBool(STRING_VALIDATE_ESCAPE, (JsonChars::is_hex(ch)), {
  validate--;
  if (!validate) {
    state=sSTRING;
  }
},{ err="Invalid unicode escape"; })

TSkipBool(KEY_START, (JsonChars::is_ws(ch)), (ch=='"'), {
  gotStart(KEY_STRING);
  state=sSTRING;
// (TODO? allowNumericKeys)
} else if (allowUnquotedKeys) {
  gotStart(KEY_UNQUOTED);
  state=sKEY_UNQUOTED;
},{
  err="Expected dict key";
})

TBool(KEY_UNQUOTED, ( (JsonChars::is_ws(ch))||(ch==':') ), {
  // TODO? synthesize '"'
  gotValue();
},{
  if (ch=='\\') {
    // BAD... TODO? synthesize another '\\'
    err="Bad unquoted key";
  }
})

TSkipBool(KEYDONE, (JsonChars::is_ws(ch)), (ch==':'), {
  state=sSTART; // DICTVALUE_START
},{
  err="Expected ':' after key";
})

TSkipBool(DICT_EMPTY, (JsonChars::is_ws(ch)), (ch=='}'), {
  gotValue();
},{ inState<sKEY_START>(ch); }) // epsilon transition

TSkipBool(ARRAY_EMPTY, (JsonChars::is_ws(ch)), (ch==']'), {
  gotValue();
},{ inState<sSTART>(ch); }) // epsilon transition

TSkipSwitch(DICT_WAIT, (JsonChars::is_ws(ch)))
TCase(',', { state=sKEY_START; })
TCase('}', { gotValue(); })
TEndSwitch({ err="Expected ',' or '}'"; }) // "instead of [ch]"

TSkipSwitch(ARRAY_WAIT, (JsonChars::is_ws(ch)))
TCase(',', { state=sSTART; })
TCase(']', { gotValue(); })
TEndSwitch({ err="Expected ',' or ']'"; })

TBool(DONE, (!JsonChars::is_ws(ch)), {
  err="Trailing character";
},{ })

void JsonState::gotStart(type_t type) // {{{
{
  stack.push_back(type);
}
// }}}

void JsonState::gotValue() // {{{
{
  assert( (!err)&&(!stack.empty()) );
  const type_t oldVal=stack.back();
  stack.pop_back();
  if (stack.empty()) {
    state=sDONE;
  } else if (isKey(oldVal)) {
    state=sKEYDONE;
  } else {
    const type_t val=stack.back();
    if (val==OBJECT) {
      state=sDICT_WAIT;
    } else if (val==ARRAY) {
      state=sARRAY_WAIT;
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
void JsonState::callState(int ch,detail::seq<S...>)
{
  static constexpr decltype(&JsonState::inState<0>) table[]={
//  static constexpr void (JsonState::*table[])(int)={
    &JsonState::inState<S>...
  };
  (this->*table[state])(ch);
}
#endif

bool JsonState::Echar(int ch) // {{{
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
  if ( (Echar(' '))&&(state!=sDONE) ) {
    err="Premature end of input";
  }
  return (!err);
}
// }}}

bool JsonState::Ekey() // {{{
{
  if (!err) {
    if ( (state==sKEY_START)||(state==sDICT_EMPTY) ) {
      state=sKEYDONE;
    } else {
      err="Unexpected key";
    }
  }
  return (!err);
}
// }}}

bool JsonState::Evalue() // {{{
{
  if (!err) {
    if ( (state==sSTART)||(state==sARRAY_EMPTY) ) {
      if (stack.empty()) {
        state=sDONE;
      } else {
        const type_t val=stack.back();
        if (val==OBJECT) {
          state=sDICT_WAIT;
        } else if (val==ARRAY) {
          state=sARRAY_WAIT;
        } else {
          assert(0);
          err="Internal error";
        }
      }
/* or    [but incompatible, when gotValue fires events]
      stack.push_back(VALUE);
      gotValue();
*/
    } else {
      err="Unexpected value";
    }
  }
  return (!err);
}
// }}}

bool JsonState::nextNumstate(char ch) // {{{
{
  using JsonChars::is_digit;
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
  static const char *type2str[]={"ARRAY", "OBJECT", "VALUE_STRING", "VALUE_NUMBER", "VALUE_BOOL", "VALUE_NULL", "KEY_STRING", "KEY_UNQUOTED"};
  // TODO? map state to string
  if (err) {
    printf("Error: %s (State: %d)\n",err,state);
  } else if (state==sDONE) {
    puts("Done");
  } else {
    printf("State: %d, Stack: [",state);
    for (size_t iA=0;iA<stack.size();iA++) {
      if (iA) {
        putchar(',');
      }
      printf("%s",type2str[stack[iA]]);
    }
    if ( (state==sVALUE_NUMBER)&&(validateNumbers) ) {
      printf("], Numstate: %d\n",numstate);
    } else {
      puts("]");
    }
  }
}
// }}}

