
// NOTE: can be used both to split and to merge surrogates

#include "utffsm.h"

namespace UtfFsm {

template <typename Output>    // bool out(int ch)
class Surrogate {
  enum { DISABLED=-1, SEEK=0 };
public:
  Surrogate(Output out) : out(out),lead(SEEK),outputAsPairs(false) {}

  void reset(bool active=true,bool writePairs=false) {
    lead=active ? SEEK : DISABLED;
    outputAsPairs=writePairs;
  }

  bool operator()(int ch) { // {{{
    if (lead==DISABLED) {
      return out(ch);
    }
    bool ret=true;
    if ( (ch>=0xdc00)&&(ch<=0xdfff) ) { // trail surrogate
      if (lead==SEEK) { // but no lead has been seen
        return out(ch | ErrorBit);
      }
      if (outputAsPairs) {
        ret&=out(lead);
        ret&=out(ch);
      } else {
        ret&=out( 0x10000 + ( ((lead&0x3ff)<<10) | (ch&0x3ff) ) );
      }
      lead=SEEK;
      return ret;
    } else if (lead!=SEEK) { // but trail was expected
      ret&=out(lead | ErrorBit); // TODO? incomplete surrogate in output (esp: not \u... anymore)
      lead=SEEK;
      // now process ch
    }
    if ( (ch>=0xd800)&&(ch<=0xdbff) ) { // lead surrogate
      lead=ch;
      // no out
    } else if (ch==EndOfInput) { // before range/split check...
      ret&=out(ch);
    } else if (ch>0x10ffff) { // not in unicode range - or: already error!
      ret&=out(ch | ErrorBit);
    } else if ( (ch>=0x10000)&&(outputAsPairs) ) { // split
      ch-=0x10000;
      ret&=out( 0xd800 | (ch>>10) );
      ret&=out( 0xdc00 | (ch&0x03ff) );
    } else {
      ret&=out(ch);
    }
    return ret;
  }
  // }}}

/*
  bool end() {
    return operator()(EndOfInput);
  }
*/

private:
  Output out;
  int lead;
  bool outputAsPairs;
};

} // namespace UtfFsm

