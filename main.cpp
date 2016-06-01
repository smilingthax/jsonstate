#include <assert.h>
#include <stdio.h>
#include "jsonstate.h"

#include <string>

bool check_json(const char *str) // {{{
{
  assert(str);
  const char *start=str;

  JsonState state;
  state.reset(true);
  state.validateNumbers=true;

  int pos;
  for (pos=0;*str;str++,pos++) {
    if (!state.Echar(*str)) {
      break;
    }
  }
  if ( (*str)||(!state.Eend()) ) {
    fprintf(stderr,"Json error at %d: %s\n",pos,state.error());
    const int prelen=std::min(str-start,20);
    fprintf(stderr,"Context: '%.*s'\n",prelen+20,str-prelen);
    fprintf(stderr,"      %s----^\n",std::string(prelen,' ').c_str());
    return false;
  }
  return true;
}
// }}}

int main(int argc,char **argv)
{
  JsonState state;

  if (argc>1) {
    return !check_json(argv[1]);
  }

  state.dump();
state.Echar('n');
state.Echar('l');
state.Echar('u');
state.Echar('l');
  state.dump();
  state.dump();

  return 0;
}
