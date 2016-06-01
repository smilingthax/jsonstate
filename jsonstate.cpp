#include "jsonstate.h"
#include <assert.h>
#include <stdio.h>

#include "jsonchars.tcc"

#if !defined(__GXX_EXPERIMENTAL_CXX0X__)&&(__cplusplus<201103L)
  #define nullptr NULL
  // can't use JsonState::type_t::type in C++03
  #define TYPE_T JsonState
#else
  #define TYPE_T JsonState::type_t
#endif

#ifdef NDEBUG
#define NDBG(code)
#else
#define NDBG(code) code
#endif

#define T_(newState) s.state=JsonState::Trans::newState; NDBG(s.stateDebug=#newState;) return true
#define T_Error(e)   s.err=e; return false
#define T_Stay       return true

#define TF(state,code) static bool state(JsonState &s,int ch) code

#define TBool(state,cond,code,elsecode) \
              TF(state,{ if cond code else elsecode })
#define TSkipBool(state,cond,code,elsecode) \
              TF(state,{ if (JsonChars::is_ws(ch)) { T_Stay; } \
                         if cond code else elsecode })

// extra braces in switch are ok... (and post-function code needs single macro)
#define TSwitch(state,code,defcode) \
              TF(state,{ switch (ch) { code \
                         default: defcode; break; } })
#define TSkipSwitch(state,code,defcode) \
              TF(state,{ if (JsonChars::is_ws(ch)) { T_Stay; } \
                         switch (ch) { code \
                         default: defcode; break; } })


struct JsonState::Trans { // inner class is implicit friend

TSkipSwitch(START, {
case '{': s.gotStart(OBJECT);       T_(DICT_EMPTY);
case '[': s.gotStart(ARRAY);        T_(ARRAY_EMPTY);
case '"': s.gotStart(VALUE_STRING); T_(STRING);
case 'f': s.gotStart(VALUE_BOOL); s.verify="alse"; T_(VALUE_VERIFY);
case 't': s.gotStart(VALUE_BOOL); s.verify="rue";  T_(VALUE_VERIFY);
case 'n': s.gotStart(VALUE_NULL); s.verify="ull";  T_(VALUE_VERIFY);
},{
  if ( (ch=='-')||(JsonChars::is_digit(ch)) ) {
    s.gotStart(TYPE_T::VALUE_NUMBER);
    if (s.validateNumbers) {
      s.numstate=NSTART;
      if (!s.nextNumstate(ch)) {
        T_Error("Invalid Number");
      }
    }
    T_(VALUE_NUMBER);
  }
  T_Error("Unexpected character (not {,[,\",t,f,n,0-9)");
})

TSkipSwitch(STRICT_START, {
case '{': s.gotStart(OBJECT); T_(DICT_EMPTY);
case '[': s.gotStart(ARRAY);  T_(ARRAY_EMPTY);
},{ T_Error("Array or Object expected"); })

TBool(VALUE_VERIFY, (ch==*s.verify), {
  s.verify++;
  if (!*s.verify) {
    return s.gotValue();
  }
  T_Stay;
},{ T_Error("Expected true/false/null keyword"); })

TBool(VALUE_NUMBER, ( (JsonChars::is_digit(ch))||(ch=='.')||(ch=='+')||(ch=='-')||(ch=='e')||(ch=='E') ), {
  if ( (s.validateNumbers)&&(!s.nextNumstate(ch)) ) {
    T_Error("Invalid Number");
  }
  T_Stay;
},{
  if ( (s.validateNumbers)&&(s.numstate!=NDONE)&&(!s.nextNumstate(0)) ) {
    T_Error("Premature end of number");
  }
  // epsilon transition - TODO?
  return (s.gotValue())&& // DONE, DICT_WAIT or ARRAY_WAIT now
         (s.Echar(ch));   // allowed next: is_ws or ",}]"
})

TSwitch(STRING, {
case '\\': T_(STRING_ESCAPE);
case '"':  return s.gotValue();
},{ T_Stay; }) // TODO? check for valid chars?

TF(STRING_ESCAPE, {
  if (s.validateEscapes) {
    if (ch=='u') {
      s.validate=4;
      T_(STRING_VALIDATE_ESCAPE);
    } else if (!JsonChars::is_escape(ch)) {
      T_Error("Invalid escape");
    } // else: fall through
  }
  T_(STRING);
});

TBool(STRING_VALIDATE_ESCAPE, (JsonChars::is_hex(ch)), {
  s.validate--;
  if (!s.validate) {
    T_(STRING);
  }
  T_Stay;
},{ T_Error("Invalid unicode escape"); })

TSkipBool(KEY_START, (ch=='"'), {
  s.gotStart(KEY_STRING);
  T_(STRING);
// (TODO? allowNumericKeys)
},{
  if (s.allowUnquotedKeys) {
    s.gotStart(TYPE_T::KEY_UNQUOTED);
    T_(KEY_UNQUOTED);
  }
  T_Error("Expected dict key");
})

TBool(KEY_UNQUOTED, ( (JsonChars::is_ws(ch))||(ch==':') ), {
  // TODO? synthesize '"'
  return s.gotValue();
},{
  if (ch=='\\') {
    // BAD... TODO? synthesize another '\\'
    T_Error("Bad unquoted key");
  }
  T_Stay;
})

TSkipBool(KEYDONE, (ch==':'), {
  T_(START); // DICTVALUE_START
},{ T_Error("Expected ':' after key"); })

TSkipBool(DICT_EMPTY, (ch=='}'), {
  return s.gotValue();
},{ return KEY_START(s,ch); }) // epsilon transition

TSkipBool(ARRAY_EMPTY, (ch==']'), {
  return s.gotValue();
},{ return START(s,ch); }) // epsilon transition

TSkipSwitch(DICT_WAIT, {
case ',': T_(KEY_START);
case '}': return s.gotValue();
},{ T_Error("Expected ',' or '}'"); }) // "instead of [ch]"

TSkipSwitch(ARRAY_WAIT, {
case ',': T_(START);
case ']': return s.gotValue();
},{ T_Error("Expected ',' or ']'"); })

TSkipBool(DONE, (true), {
  T_Error("Trailing character");
},{ })

}; // struct JsonState::Trans

#define To(newState) state=JsonState::Trans::newState; NDBG(stateDebug=#newState)
#define In(otherState) (state==JsonState::Trans::otherState)

bool JsonState::inWait() const
{
  return (!err)&&( In(DICT_WAIT)||In(ARRAY_WAIT) );
}

bool JsonState::done() const
{
  return (!err)&&(In(DONE));
}

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


void JsonState::gotStart(type_t type) // {{{
{
  stack.push_back(type);
}
// }}}

static inline bool isKey(JsonState::type_t type) // {{{
{
  return (type==TYPE_T::KEY_STRING)||(type==TYPE_T::KEY_UNQUOTED);
}
// }}}

bool JsonState::gotValue() // {{{
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
  return (!err);
}
// }}}


bool JsonState::Echar(int ch) // {{{
{
  if (err) {
    return false;
  }
  // assert(state);
  return (*state)(*this,ch);
  // assert( (return==true) || err );
}
// }}}

bool JsonState::Eend() // {{{
{
  if ( (!err)&&(Echar(' '))&&(!In(DONE)) ) { // TODO? ' ' produces "better" error message (but is not recoverable)
    err="Premature end of input";
  }
  return (!err);
}
// }}}

bool JsonState::Ekey() // {{{
{
  if (!err) {
    if ( In(KEY_START)||In(DICT_EMPTY) ) {
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
    if ( In(START)||In(ARRAY_EMPTY) ) {
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

const char *JsonState::typeName(JsonState::type_t type) // {{{
{
  static const char *type2str[]={"ARRAY", "OBJECT", "VALUE_STRING", "VALUE_NUMBER", "VALUE_BOOL", "VALUE_NULL", "KEY_STRING", "KEY_UNQUOTED"};
  return type2str[type];
}
// }}}

void JsonState::dump() const // {{{
{
  if ( (!err)&&(In(DONE)) ) {
    puts("Done");
    return;
  }
  if (err) {
    printf("Error: %s\n",err);
  }
#ifdef NDEBUG
  printf("State: %p, Stack: [",state);
#else
  printf("State: %s, Stack: [",stateDebug);
#endif
  for (size_t iA=0;iA<stack.size();iA++) {
   if (iA) {
      putchar(',');
    }
    printf("%s",typeName(stack[iA]));
  }
  if ( In(VALUE_NUMBER)&&(validateNumbers) ) {
    printf("], Numstate: %d\n",numstate);
  } else {
    puts("]");
  }
}
// }}}

