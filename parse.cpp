#include <stdio.h>
#include "jsonparser.h"
#include "jsonfilewriter.h"

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
  for (pos=0;*str;str++,pos++) {  // TODO... read utf8  - but JsonState works even undecoded   [not validated]
    if (!parser.next(*str)) {
      break;
    }
  }

  const char *err=parser.end();
  printf("\n");
  if (err) {
    fprintf(stderr,"Json error at %d: %s\n",pos,err);
    const int prelen=std::min(str-argv[1],20);
    fprintf(stderr,"Context: '%.*s'\n",prelen+20,str-prelen);
    fprintf(stderr,"      %s----^\n",std::string(prelen,' ').c_str());
    return 1;
  }

  return 0;
}
