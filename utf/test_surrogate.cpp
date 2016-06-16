#include <stdio.h>
#include "utfsurrogate.tcc"

bool outhex(int ch) {
  using UtfFsm::ErrorBit;
  if (ch==UtfFsm::EndOfInput) {
    printf("EOI\n");
    return true;
  } else if (ch&ErrorBit) {
    printf("[%04x] ",ch^ErrorBit);
    return false;
  } else {
    printf("%04x ",ch);
    return true;
  }
}

int main()
{
  UtfFsm::Surrogate<decltype(&outhex)> sfsm(outhex);

  sfsm.reset(true,true);
  if (!sfsm(0x1d11e)) printf("\nerror\n");
  if (!sfsm(UtfFsm::EndOfInput)) printf("\nerror\n");

  // --
  sfsm.reset();
  if (!sfsm(0xd801)) printf("\nerror\n");
  if (!sfsm(0xdc00)) printf("\nerror\n");
  if (!sfsm(UtfFsm::EndOfInput)) printf("\nerror\n");

  return 0;
}

