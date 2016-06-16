#include <stdio.h>
#include "jsunescape.tcc"

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
  JsUnescapeFSM<decltype(&outhex)> unesc(outhex);

//unesc.reset(true,true);
//  for (auto ch : "\\n \\u12Abx \\udc01\\ud800") {
//  for (auto ch : "\\n \\u1234x \\udb00 \\udc00") {
  for (auto ch : "\\n \\u12Abx \\ud801\\udc00") {
//  for (auto ch : "\\n \\u12Abx \\ud81\\udc00") {
    if (!unesc(ch ? ch : UtfFsm::EndOfInput)) {
//    if (!unesc(ch)) {
      printf("\nerror\n");
//      break;
    }
  }

  printf("\n");

  return 0;
}

