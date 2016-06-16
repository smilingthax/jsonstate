
#include "utffsm.h"

namespace UtfFsm {

// bool out(int ch)
template <typename Output,bool allowCesu8=false,bool allowOverlong=false>
class Utf8Decode {
  enum { DISABLED=-1, SEEK=0 };
public:
  Utf8Decode(Output out) : out(out),state(SEEK) {}

  // TODO... use Bypass<> instead of DISABLED (remove!)
  void reset(bool active=true) {
    state=active ? SEEK : DISABLED;
  }

  bool operator()(int ch) { // {{{
    if (state==DISABLED) {
      return out(ch);
    }
    bool ret=true;
    if ( (ch<0)||(ch>0xff) ) { // not a regular char
      if (state!=SEEK) {
        ret&=flushIncompleteState(state);
        state=SEEK;
      }
      ret&=out(ch | ErrorBit); // Error, EndOfInput(HACK!!)
    } else if (ch&0x80) { // non-ASCII
      if ((ch&0x40)==0) { // continuation byte
        if (state==SEEK) {
          return out(ch | ErrorBit); // unexpected continuation
        }
        // 00000000 00000000 00000000 00000000
        //          aaaaaabb bbbbcccc ccdddddd
        //   pp ???    ...    ...   l ll         (fmt: pp00 0000 0lll)
        // (pp is from original prefix, used in flushIncompleteState)
        state=(state<<6) | (ch&0x3f);
        const int len=state&0x07000000;
        if (len) { // done
          ret=cout(state&0xffffff,len>>24); // not: state>>24 (-> 64bit!)
          state=SEEK;
        } // else: wait for more
        return ret;
      } else if (state!=SEEK) { // unexpected non-continuation
        ret&=flushIncompleteState(state);
        state=SEEK;
        // now process ch
      }
      if ((ch&0x20)==0) { // len 2
        state=(0x002 << 18) | (ch&0x1f);
      } else if ((ch&0x10)==0) { // len 3
        state=(0x803 << 12) | (ch&0x0f);
      } else if ((ch&0x08)==0) { // len 4
        state=(0xc04 << 6) | (ch&0x07);
      } else { // len 5/6 not allowed
        ret&=out(ch | ErrorBit); // (leaves state==SEEK)
      }
    } else { // ASCII
      if (state!=SEEK) { // but continuation expected
        ret&=flushIncompleteState(state);
        state=SEEK;
        // now process ch
      }
      ret&=out(ch); // not cout(), ascii is always valid
    }
    return ret;
  }
  // }}}

private:
  Output out;
  int state;
  bool cout(int ch,int len) { // {{{    TODO? encode back?
    if ( (len>1)&&(!allowOverlong) ) {
      if ( (len==2)&&(ch==0)&&(allowCesu8) ) {
        return out(ch);
      } else if ( (len>1)&&(ch<=0x7f) ) {
        return out(ch | ErrorBit);
      } else if ( (len>2)&&(ch<=0x7ff) ) {
        return out(ch | ErrorBit);
      } else if ( (len>3)&&(ch<=0xffff) ) {
        return out(ch | ErrorBit);
      }
    }
    if (ch>=0x10ffff) { // not in range
      return out(ch | ErrorBit); // todo? encode back?
    }
    // ... will not check for surrogates / 0xfffe / 0xffff    FIXME?
    return out(ch);
  }
  // }}}
  bool flushIncompleteState(int state) { // {{{
    // assert(state>0);
    // assert((state&~0x30ffffff)==0); // (even for 64bit!)
    // 00pp0000 ..0lllaa aaaabbbb bbcccccc

    int num=-1; // (at least 1 missing)
    while ((state&0x001c0000)==0) {
      state<<=6;
      num--;
    }
    num+=(state&0x001c0000)>>18; // i.e. num+=len;
    // assert( (num>=1)&&(num<=3) );

    const int prefix=0xc0 | (state>>24);
    bool ret=out(prefix | ((state>>12)&0x3f) | ErrorBit); // or: 0x1f
#if 0 // (compiler probably can't deduce 1 <= num <= 3)
    for (num--; num>0; num--) {
      state<<=6;
      ret&=out(0x80 | ((state>>12)&0x3f) | ErrorBit);
    }
#else // unrolled
    if (num==3) {
      ret&=out(0x80 | ((state>>6)&0x3f) | ErrorBit);
      ret&=out(0x80 | (state&0x3f) | ErrorBit);
    } else if (num==2) {
      ret&=out(0x80 | ((state>>6)&0x3f) | ErrorBit);
    }
#endif
    return ret;
  }
  // }}}
};


template <typename Output> // void out(start,end)
bool writeUtf8(Output out,int ch,unsigned char len) { // {{{
  char buf[4];
  if (len==1) {
    buf[0]=ch;  // or:  overloaded  out((unsigned char)ch); ?
  } else if (len<=4) {
    for (int i=len-1; i>0; i--) {
      buf[i]=0x80 | (ch&0x3f);
      ch>>=6;
    }
    const unsigned char prefix=(0xf0<<(4-len));
    buf[0]=prefix | ch;
  } else {
    return false;
  }
// buf[len]=0; // be nice?
  out(buf,buf+len);
  return true;
}
// }}}

// TODO?! enclen(ch) ?
template <typename Output> // TODO c++11: perfect forwarding...
bool writeUtf8(Output out,int ch) { // {{{
  if (ch<0) {
    return false;
  } else if (ch<=0x7f) {
    return writeUtf8<Output&>(out,ch,1);
  } else if (ch<=0x7ff) {
    return writeUtf8<Output&>(out,ch,2);
  } else if (ch<=0xffff) {
    return writeUtf8<Output&>(out,ch,3);
  } else if (ch<=0x10ffff) {
    return writeUtf8<Output&>(out,ch,4);
  }
  return false; // too large
}
// }}}

template <typename Output> // void out(start,end)
struct Utf8Writer {
  Utf8Writer(Output out) : out(out) {}

  bool operator()(int ch) {
    if (ch==EndOfInput) {
      return true;
    }
    // TODO/FIXME: handle  (ch&ErrorBit)  ,  handle (ch==EndOfInput)        [currently: return false...(via out-of-range)]
    return writeUtf8<Output&>(out,ch);
  }

private:
  Output out;
};

} // namespace UtfFsm
