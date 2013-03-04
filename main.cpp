#include <assert.h>
#include <stdio.h>
#include "jsonstate.h"

bool check_json(const char *str) // {{{
{
  assert(str);
  JsonState state;
  state.reset(true);
  state.validateNumbers=true;

  int iA;
  for (iA=0;*str;str++,iA++) {
    if (!state.Echar(*str)) {
      fprintf(stderr,"Json error at %d: %s\n",iA,state.error());
      return false;
    }
  }
  if (!state.Eend()) { // in case of numbers
    fprintf(stderr,"Json error at %d: %s\n",iA,state.error());
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
