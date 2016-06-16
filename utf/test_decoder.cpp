#include "utf8.tcc"
#include <stdio.h>
#include <string.h>

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
  const char *str;

//  UtfFsm::Utf8Decode<decltype(&outhex)> decoder(outhex);
  UtfFsm::Utf8Decode<decltype(&outhex),true> decoder(outhex); // allow cesu

  str="a\xff_b\n_cä\x80_d\xa0\x80_e_f\xc0";
  for (const char *pos=str; *pos; ++pos) {
    if (!decoder((unsigned char)*pos)) {
      printf("\nerror\n");
    }
  }
if (!decoder(0x80)) printf("\nerror\n");
  if (!decoder(UtfFsm::EndOfInput)) {
    printf("\nerror\n");
  }
//  printf("\n");

  return 0;
}

