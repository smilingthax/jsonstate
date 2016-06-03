#include <stdio.h>
#include "jsonstate.h"

struct LimitDepth : JsonState::EventListenerBase {
  LimitDepth(int depth) : depth(depth) {}

  const char *startValue(JsonState &state,JsonState::type_t type) { // override
    if ( (type==JsonState::ARRAY)||(type==JsonState::OBJECT) ) {
      depth--;
      if (depth==0) {
        return "Recursion limit reached";
      }
    }
    return nullptr;
  }

  void endValue(JsonState &state,JsonState::type_t type) { // override
    if ( (type==JsonState::ARRAY)||(type==JsonState::OBJECT) ) {
      depth++;
    }
  }

private:
  int depth;
};

bool check(FILE *f)
{
  JsonState state;
  state.reset(true);
  state.validateEscapes=true;
  state.validateNumbers=true;
//state.allowUnquotedKeys=true;

  LimitDepth limiter(20);
  state.setListener(&limiter);

  int pos=0,c;
  while ((c=fgetc(f))!=EOF) {
    pos++;
// FIXME: decode utf8
    if (!state.Echar(c)) {
      break;
    }
  }
  if (!state.Eend()) {
    printf("Json error at %d: %s\n",pos,state.error());
//puts("");state.dump();
    return false;
  }
  return true;
}

int main(int argc,char **argv)
{
  if (!check(stdin)) {
    return 1;
  }

  return 0;
}
