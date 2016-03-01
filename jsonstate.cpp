#include "jsonstate.h"
#include <assert.h>
#include <stdio.h>

#include "jsonchars.tcc"

#if !defined(__GXX_EXPERIMENTAL_CXX0X__)&&(__cplusplus<201103L)
  #define nullptr NULL
  // can't use JsonState::state_t::state in C++03
  #define STATE_T JsonState
#else
  #define STATE_T JsonState::state_t
#endif


JsonState::JsonState()
  : allowUnquotedKeys(false),
    validateEscapes(false),
    validateNumbers(false),
    err(nullptr),state(&JsonState::sSTART),stateDebug("START")
{
}

#define To(newState) state=&JsonState::s ## newState; stateDebug=#newState

void JsonState::reset(bool strictStart) // {{{
{
  err=nullptr;
  stack.clear();
  if (strictStart) {
    To(STRICT_START);
  } else {
    To(START);
  }
}
// }}}


#define TF(state,code) void JsonState::s ## state(int ch) code

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
TCase('{', { gotStart(OBJECT);       To(DICT_EMPTY); })
TCase('[', { gotStart(ARRAY);        To(ARRAY_EMPTY); })
TCase('"', { gotStart(VALUE_STRING); To(STRING); })
TCase('f', { gotStart(VALUE_BOOL);   To(VALUE_VERIFY); verify="alse"; })
TCase('t', { gotStart(VALUE_BOOL);   To(VALUE_VERIFY); verify="rue"; })
TCase('n', { gotStart(VALUE_NULL);   To(VALUE_VERIFY); verify="ull"; })
TEndSwitch({
  if ( (ch=='-')||(JsonChars::is_digit(ch)) ) {
    gotStart(VALUE_NUMBER);
    To(VALUE_NUMBER);
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
TCase('{', { gotStart(OBJECT); To(DICT_EMPTY); })
TCase('[', { gotStart(ARRAY);  To(ARRAY_EMPTY); })
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
TCase('\\',{ To(STRING_ESCAPE); })
TCase('"', { gotValue(); })
TEndSwitch({ }) // TODO? check for valid chars?

TF(STRING_ESCAPE, {
  To(STRING);
  if (validateEscapes) {
    if (ch=='u') {
      To(STRING_VALIDATE_ESCAPE);
      validate=4;
    } else if (!JsonChars::is_escape(ch)) {
      err="Invalid escape";
    }
  }
});

TBool(STRING_VALIDATE_ESCAPE, (JsonChars::is_hex(ch)), {
  validate--;
  if (!validate) {
    To(STRING);
  }
},{ err="Invalid unicode escape"; })

TSkipBool(KEY_START, (JsonChars::is_ws(ch)), (ch=='"'), {
  gotStart(KEY_STRING);
  To(STRING);
// (TODO? allowNumericKeys)
} else if (allowUnquotedKeys) {
  gotStart(KEY_UNQUOTED);
  To(KEY_UNQUOTED);
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
  To(START); // DICTVALUE_START
},{
  err="Expected ':' after key";
})

TSkipBool(DICT_EMPTY, (JsonChars::is_ws(ch)), (ch=='}'), {
  gotValue();
},{ sKEY_START(ch); }) // epsilon transition

TSkipBool(ARRAY_EMPTY, (JsonChars::is_ws(ch)), (ch==']'), {
  gotValue();
},{ sSTART(ch); }) // epsilon transition

TSkipSwitch(DICT_WAIT, (JsonChars::is_ws(ch)))
TCase(',', { To(KEY_START); })
TCase('}', { gotValue(); })
TEndSwitch({ err="Expected ',' or '}'"; }) // "instead of [ch]"

TSkipSwitch(ARRAY_WAIT, (JsonChars::is_ws(ch)))
TCase(',', { To(START); })
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
    To(DONE);
  } else if (isKey(oldVal)) {
    To(KEYDONE);
  } else {
    const type_t val=stack.back();
    if (val==OBJECT) {
      To(DICT_WAIT);
    } else if (val==ARRAY) {
      To(ARRAY_WAIT);
    } else {
      assert(0);
      err="Internal error";
    }
  }
}
// }}}


bool JsonState::Echar(int ch) // {{{
{
  if (err) {
    return false;
  }
  // assert(state);
  (this->*state)(ch);

  return (!err);
}
// }}}

bool JsonState::Eend() // {{{
{
  if ( (Echar(' '))&&(state!=&JsonState::sDONE) ) { // TODO? ' ' produces "better" error message (but is not recoverable)
    err="Premature end of input";
  }
  return (!err);
}
// }}}

bool JsonState::Ekey() // {{{
{
  if (!err) {
    if ( (state==&JsonState::sKEY_START)||(state==&JsonState::sDICT_EMPTY) ) {
      To(KEYDONE);
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
    if ( (state==&JsonState::sSTART)||(state==&JsonState::sARRAY_EMPTY) ) {
      if (stack.empty()) {
        To(DONE);
      } else {
        const type_t val=stack.back();
        if (val==OBJECT) {
          To(DICT_WAIT);
        } else if (val==ARRAY) {
          To(ARRAY_WAIT);
        } else {
          assert(0);
          err="Internal error";
        }
      }
/* basically:    [but incompatible, when gotValue fires events]
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
    printf("Error: %s (State: %s)\n",err,stateDebug);
  } else if (state==&JsonState::sDONE) {
    puts("Done");
  } else {
    printf("State: %s, Stack: [",stateDebug);
    for (size_t iA=0;iA<stack.size();iA++) {
      if (iA) {
        putchar(',');
      }
      printf("%s",type2str[stack[iA]]);
    }
    if ( (state==&JsonState::sVALUE_NUMBER)&&(validateNumbers) ) {
      printf("], Numstate: %d\n",numstate);
    } else {
      puts("]");
    }
  }
}
// }}}

