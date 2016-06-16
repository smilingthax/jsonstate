#ifndef _UTFFSM_H
#define _UTFFSM_H

namespace UtfFsm {
enum { ErrorBit=0x01000000, EndOfInput=-1 };
    // TODO? or:  out(int ch,bool error) ??

template <typename Output,bool initialActive=true>
struct Bypass {
  Bypass(Output out) : out(out),active(initialActive) {}

  void reset(bool _active) {  // =initialActive ?
    active=_active;
  }

  bool operator()(int ch) {
    if (!active) {
      return true;
    }
    return out(ch);
  }

private:
  Output out;
  bool active;
};

} // namespace UtfFsm

#endif
