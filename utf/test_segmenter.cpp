#include "utf8segmenter.tcc"
#include <stdio.h>
#include <string.h>

struct DebugInterface {
  bool mustEscape(char ch) {
    return (ch<0x20);
  }
  void writeEscape(char ch) {
    printf("0x%02x ",ch);
  }
  void write(const char *start,const char *end) {
    printf("%.*s ",end-start,start);
  }
  void writeUtf8(int ch,const char *start,const char *end) {
    printf("%04x[",ch);
    for (; start!=end; ++start) {
      printf("%02x ",(unsigned char)*start);
    }
    printf("] ");
  }
  bool error(const char *start,const char *end) {
    printf("[bad: ");
    for (; start!=end; ++start) {
      printf("%02x ",(unsigned char)*start);
    }
    printf("] ");
    return false;
  }
};

int main()
{
  const char *str;

  str="a\xff_b\n_cä\x80_d\xa0\x80_e_f\xc0";
  utf8segmenter(DebugInterface(),str,str+strlen(str));
  printf("\n\n");

  Utf8Segmenter<DebugInterface> seg;
  seg(str,str+strlen(str),false);
  str="\x80\xc0";
  seg(str,str+2,false);
  seg(str,str+1);
  printf("\n");

  return 0;
}

