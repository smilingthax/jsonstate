#include <stdio.h>
#include "jsonparser.h"
#include "jsonfilewriter.h"

#include "utf/utf8segmenter.tcc"

int main(int argc,char **argv)
{
  if (argc<2) {
    fprintf(stderr,"Usage: %s json\n",argv[0]);
    return 1;
  }
  const char *str=argv[1];

//  JsonBasicFileWriter writer;
  JsonPrettyFileWriter writer;
  JsonParser parser(writer);

  int pos;
  for (pos=0; *str; str++,pos++) {  // TODO... better utf8?
    if (*str&0x80) {
      const int res=scan_one_utf8(str);
      if ( (res<0)||(!parser.next(res)) ) {
        break;
      }
      str--; // already at next char
    } else if (!parser.next(*str)) {
      break;
    }
  }

  const char *err=parser.end();
  printf("\n");
  if (err) { // FIXME: not utf8 safe ...
    fprintf(stderr,"Json error at %d: %s\n",pos,err);
    const int prelen=std::min(str-argv[1],20);
    fprintf(stderr,"Context: '%.*s'\n",prelen+20,str-prelen);
    fprintf(stderr,"      %s----^\n",std::string(prelen,' ').c_str());
    return 1;
  }

  return 0;
}
