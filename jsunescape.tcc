
#include "utf/utffsm.h"
#include "utf/utfsurrogate.tcc"

template <typename Output>  // bool out(int ch)
struct JsUnescapeFSM {
  enum { DISABLED=-3, SEEK=-2, WAIT=-1, START=0 };
  JsUnescapeFSM(Output out) : out(out),outSurrogate(this->out),state(SEEK) {}

  void reset(bool active=true,bool asSurrogates=false) {
    state=active ? SEEK : DISABLED;
    outSurrogate.reset(true,asSurrogates);
  }

  bool operator()(int ch) { // {{{
    using UtfFsm::ErrorBit;
    bool ret=true;
    switch (state) {
    case DISABLED:
      return out(ch);   // bypasses surrogates (TODO?)
    case SEEK:
      if (ch=='\\') {
        state=WAIT;
        return true;
      }
      return outSurrogate(ch);
    case WAIT:
      if (ch=='u') {
        state=START;
        save=0x80808080;
        return true;
      }
      state=SEEK;
      if ( (ch==UtfFsm::EndOfInput)||((ch&ErrorBit)==0) ) {
        const int res=shortCodes(ch);
        if (res) {
          return outSurrogate(res);
        }
      }
      ret&=outSurrogate('\\' | ErrorBit);
      ret&=outSurrogate(ch);  // note: (ch!='\\')
      return ret;
    default: // >=START
      if ( (ch!=UtfFsm::EndOfInput)&&((ch&ErrorBit)==0) ) {
        const int res=unhex(ch);
        if (res>=0) {
          state=(state<<4)|res;
          save=(save<<8)|ch;  // (ch<0x80)
          if (save&0x80000000) {
            return true; // more to read
          } // else: complete
          ret=outSurrogate(state);
          state=SEEK;
          return ret;
        } // else: error
      } // else: error
      ret&=outSurrogate('\\' | ErrorBit);
      ret&=outSurrogate('u' | ErrorBit);
      while (save&0x80000000) {
        save<<=8;
      }
      while (save&=0xffffffff) { // could be 64bit!
        ret&=outSurrogate((save>>24) | ErrorBit);
        save<<=8;
      }
      // now handle ch as if in state==SEEK
      if (ch=='\\') {
        state=WAIT;
      } else {
        ret&=outSurrogate(ch);
        state=SEEK;
      }
      return ret;
    }
  }
  // }}}

private:
  Output out;
  UtfFsm::Surrogate<Output&> outSurrogate;

  int state;
  int save;

  static inline int shortCodes(int ch) { // {{{   \" \/ \\ \b \f \n \r \t   else 0
    switch (ch) {
    case '"': case '/': case '\\': return ch;
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    default: return 0;
    }
  }
  // }}}
  static inline int unhex(int ch) { // {{{  or -1
    if ( (ch>='0')&&(ch<='9') ) {
      return ch-'0';
    } else if ( (ch>='A')&&(ch<='F') ) {
      return ch-'A'+10;
    } else if ( (ch>='a')&&(ch<='f') ) {
      return ch-'a'+10;
    }
    return -1;
  }
  // }}}
};

