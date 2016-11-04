
// To be called on byte with 0x80 set (pos<end !)
// NOTE: end==nullptr can be used for \0 terminated strings
// NOTE: requires 4 bytes at the most
// NOTE: overlong encodings are not even detected
// returns next codepoint, or -1 (too long, incomplete, 0x80 not set, bad)
// NOTE: will increment pos at least by one
static inline int scan_one_utf8(const char *&pos,const char *end=0) // {{{
{
  // assert(pos<end);
  const unsigned char ch0=*pos;
  ++pos;
  if ( ((ch0&0xc0)!=0xc0)||
       (pos==end)||((*pos&0xc0)!=0x80) ) {
    return -1;
  }
  int val=*pos&0x3f;
  ++pos;
  if ((ch0&0x20)==0) { // len 2
    val=((ch0&0x1f)<<6) | val;
  } else if ( (pos==end)||((*pos&0xc0)!=0x80) ) {
    return -1;
  } else { // len >=3
    val=((ch0&0xf)<<6) | val;
    val=(val<<6) | (*pos&0x3f);
    ++pos;
    if (ch0&0x10) { // len>=4
      if ( (ch0&0x08)||
           (pos==end)||((*pos&0xc0)!=0x80) ) {
        return -1;
      }
      val=(val<<6)|(*pos&0x3f);
      ++pos;
    }
  }
  return val;
}
// }}}

static inline bool overlong_utf8(int ch,int len,bool allowCesu8=false) { // {{{
  if (ch<0) {
    return true; // TODO?
  } else if (ch<=0x7f) {
    return (len!=1)&&( (!allowCesu8)||(len!=2)||(ch!=0) );
  } else if (ch<=0x7ff) {
    return (len!=2);
  } else if (ch<=0xffff) {
    return (len!=3);
  } else if (ch<=0x10ffff) {
    return (len!=4);
  } else { // out of range
    return true;
  }
}
// }}}

static inline const char *next_utf8_boundary(const char *pos,const char *end=0) // {{{
{
  for (; pos!=end; ++pos) {
    if ((*pos&0xc0)!=0x80) { // ((*pos&0x80)==0)||(*pos&0x40)
      break;
    }
  }
  return pos;
}
// }}}

