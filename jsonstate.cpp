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

#define EOI (-1)   // end-of-input  (EOF already used by stdio.h)

#define T_(newState) s.state=JsonState::Trans::newState; NDBG(s.stateDebug=#newState;) return true
#define T_Epsilon(newState) s.state=JsonState::Trans::newState; NDBG(s.stateDebug=#newState "(epsilon)";) return newState(s,ch)
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
case '{': s.gotStart(OBJECT); s.first=true; T_(DICT_EMPTY);
case '[': s.gotStart(ARRAY);  s.first=true; T_(ARRAY_EMPTY);
case '"': s.gotStart(VALUE_STRING); T_(STRING);
case 'f': s.gotStart(VALUE_FALSE); s.verify="alse"; T_(VALUE_VERIFY);
case 't': s.gotStart(VALUE_TRUE);  s.verify="rue";  T_(VALUE_VERIFY);
case 'n': s.gotStart(VALUE_NULL);  s.verify="ull";  T_(VALUE_VERIFY);
},{
  if ( (ch=='-')||(JsonChars::is_digit(ch)) ) {
    s.gotStart(TYPE_T::VALUE_NUMBER);
    if ( (!s.err)&&(s.validateNumbers) ) {
      s.numstate=NSTART;
      if (!s.nextNumstate(ch)) {
        T_Error("Invalid Number");
      }
    }
    T_(VALUE_NUMBER);
  }
  T_Error("Unexpected character (not {,[,\",t,f,n,-,0-9)");
})

TSkipSwitch(STRICT_START, {
case '{': s.gotStart(OBJECT); s.first=true; T_(DICT_EMPTY);
case '[': s.gotStart(ARRAY);  s.first=true; T_(ARRAY_EMPTY);
},{ T_Error("Array or Object expected"); })

TBool(VALUE_VERIFY, (ch==*s.verify), {
  s.verify++;
  if (!*s.verify) {
    T_(GOT_VALUE);
  }
  T_Stay;
},{ T_Error("Expected true/false/null keyword"); })

TBool(VALUE_NUMBER, ( (JsonChars::is_digit(ch))||(ch=='.')||(ch=='+')||(ch=='-')||(ch=='e')||(ch=='E') ), {
  if ( (s.validateNumbers)&&(!s.nextNumstate(ch)) ) { // ch is ASCII, int->char is safe
    T_Error("Invalid Number");
  }
  T_Stay;
},{
  if ( (s.validateNumbers)&&(s.numstate!=NDONE)&&(!s.nextNumstate(0)) ) {
    T_Error("Premature end of number");
  }
  T_Epsilon(GOT_VALUE); // no GOT_VALUE delay
})

TSwitch(STRING, {
case '\\': T_(STRING_ESCAPE);
case '"':  T_(GOT_VALUE);
case EOI:  T_Error("Unexpected end while reading quoted string"); // TODO? error output: skip back to where string started?
},{
  if (JsonChars::is_control0(ch)) {
    T_Error("Raw control characters are not allowed in quoted string");
  }
  T_Stay;
})

TF(STRING_ESCAPE, {
  if (ch==EOI) {
    T_Error("Unexpected end while processing \\ in quoted string");
  } else if (s.validateEscapes) {
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
},{
// (TODO? allowNumericKeys)
  if ( (s.allowUnquotedKeys)&&(JsonChars::is_quotefree(ch)) ) {
    s.gotStart(TYPE_T::KEY_UNQUOTED);
    T_(KEY_UNQUOTED);
  }
  T_Error("Expected dict key");
})

TBool(KEY_UNQUOTED, (JsonChars::is_quotefree(ch)), {
  T_Stay;
},{ T_Epsilon(GOT_VALUE); }); // (-> KEYDONE)

TSkipBool(DICT_EMPTY, (ch=='}'), {
  T_(GOT_VALUE);
},{ T_Epsilon(KEY_START); })

TSkipBool(ARRAY_EMPTY, (ch==']'), {
  T_(GOT_VALUE);
},{ T_Epsilon(START); })

TF(GOT_VALUE, {
  const type_t prevType=s.gotValue();
  if ( (prevType==TYPE_T::KEY_STRING)||(prevType==TYPE_T::KEY_UNQUOTED) ) {
    T_Epsilon(KEYDONE); // expects ":" (or ws)
  }
  T_Epsilon(STACK); // expects ",}]" (or ws)
})

// the remaining states are those reached by (and only by) gotValue/Evalue

TSkipBool(KEYDONE, (ch==':'), {
  s.first=true;
  T_(START); // DICTVALUE_START
},{ T_Error("Expected ':' after key"); })

// helper: use state according to top-of-stack type (DONE/DICT_WAIT/ARRAY_WAIT)
TF(STACK, {
  s.first=false;
  if (s.stack.empty()) {
    T_Epsilon(DONE);
  } else {
    const type_t type=s.stack.back();
    if (type==OBJECT) {
      T_Epsilon(DICT_WAIT);
    } else if (type==ARRAY) {
      T_Epsilon(ARRAY_WAIT);
    }
    assert(0);
    T_Error("Internal error");
  }
})

TSkipSwitch(DICT_WAIT, {
case ',': T_(KEY_START);
case '}': T_(GOT_VALUE);
},{ T_Error("Expected ',' or '}'"); }) // "instead of [ch]"

TSkipSwitch(ARRAY_WAIT, {
case ',': T_(START);
case ']': T_(GOT_VALUE);
},{ T_Error("Expected ',' or ']'"); })

TSkipBool(DONE, (ch==EOI), {
  T_Stay;
},{ T_Error("Trailing character"); });

}; // struct JsonState::Trans

#define To(newState) state=JsonState::Trans::newState; NDBG(stateDebug=#newState)
#define In(otherState) (state==JsonState::Trans::otherState)

/* FIXME   epsilons from GOT_VALUE...
bool JsonState::inWait() const
{
  return (!err)&&( In(DICT_WAIT)||In(ARRAY_WAIT) );
}
*/

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
  first=true;
}
// }}}

void JsonState::setListener(EventListenerBase *listen) // {{{
{
  fire=listen;
}
// }}}


// called from START, STRICT_START, KEY_START
// and (via epsilon) from DICT_EMPTY, ARRAY_EMPTY
void JsonState::gotStart(type_t type) // {{{
{
  assert(!err);
  stack.push_back(type);
  if (fire) {
    err=fire->startValue(*this,type);
  }
}
// }}}

JsonState::type_t JsonState::gotValue() // {{{
{
  assert( (!err)&&(!stack.empty()) );
  const type_t prevType=stack.back();
  stack.pop_back();
  if (fire) {
    fire->endValue(*this,prevType);
  }
  return prevType;
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
  if ( (!err)&&(Echar(EOI))&&(!In(DONE)) ) {
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
  first=false;
  return (!err);
}
// }}}

bool JsonState::Evalue() // {{{
{
  if (!err) {
    if ( In(START)||In(ARRAY_EMPTY) ) {
      // i.e.  GOT_VALUE  w/o  events    (hint: for events, could use  GOT_START(*this,' ') ...)
      To(STACK);
    } else {
      err="Unexpected value";
    }
  }
  first=false;
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
  static const char *type2str[]={
    "ARRAY", "OBJECT",
    "VALUE_STRING", "VALUE_NUMBER", "VALUE_FALSE", "VALUE_TRUE", "VALUE_NULL",
    "KEY_STRING", "KEY_UNQUOTED"};
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

