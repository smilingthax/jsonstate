
/*
template <typename Output>
struct SegmenterInterface {
  // SegmenterInterface(Output out) : out(out) {}

  bool mustEscape(char ch) {
    return false;
  }
  void writeEscape(char ch) {
    // ... out(...)
  }
  void write(const char *start,const char *end) {
    // ... out(...)
  }

protected:
  Output out;
};

template <typename Output>
struct Utf8SegmenterInterface : SegmenterInterface<Output> {
  void writeUtf8(int ch,const char *start,const char *end) {
    // ? ... check overlong / inRange / ... ?
    // ? ... mustEscape?
    // ... out(...)
  }
  bool error(const char *start,const char *end) {
    // contains only bytes with high byte set
    // if a start-sequence is included, it begins at *start;
    // it can only be too short when end came first
    return true;
  }
};
*/

// esp. included to facilitate understanding
template <typename Segments>
void latin1segmenter(Segments seg,const char *start,const char *end) // {{{
{
  const char *pos=start;
  while (pos!=end) {
    const unsigned char ch=*pos;
    if (seg.mustEscape(ch)) { // (single byte only)
      if (start!=pos) {
        seg.write(start,pos);
      }
      seg.writeEscape(ch);
      start=++pos;
    } else {
      ++pos;
    }
  }
  if (start!=pos) {
    seg.write(start,pos);
  }
}
// }}}

// To be called on bytes with 0x80 set.
// NOTE: end==nullptr can be used with \0 termination
static inline int scan_one_utf8(const char *&pos,const char *end=0) // {{{
{
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

template <typename Segments>
bool utf8segmenter(Segments seg,const char *start,const char *end) // {{{
{
  const char *pos=start;
  while (pos!=end) {
    const unsigned char ch=*pos;
    if (ch&0x80) { // multi byte sequence
      if (start!=pos) {
        seg.write(start,pos);   // todo?  bool out(start,end) ?
        start=pos;
      }
      const int val=scan_one_utf8(pos,end);
      if (val>=0) {
        seg.writeUtf8(val,start,pos);
        start=pos;
      } else { // error
        for (; pos!=end; ++pos) { // skip to next sync
          if ( ((*pos&0x80)==0)||(*pos&0x40) ) {
            break;
          }
        }
        if (seg.error(start,pos)) {
          return false;
        }
        start=pos;
      }
    } else if (seg.mustEscape(ch)) { // (single byte only)
      if (start!=pos) {
        seg.write(start,pos);
      }
      seg.writeEscape(ch);
      start=++pos;
    } else {
      ++pos;
    }
  }
  if (start!=pos) {
    seg.write(start,pos);
  }
  return true;
}
// }}}


template <typename Segments>
struct Utf8Segmenter {
  Utf8Segmenter(Segments seg=Segments()) : wrap(seg) {}  // TODO c++11: forward ?

//  void reset() { wrap.pos=0; }

  bool operator()(const char *start,const char *end,bool endOfInput=true) { // {{{
    wrap.curEnd=0; // c++11: nullptr
    if (wrap.pos) {
      const char *wrapend=&wrap.leftover[4];
      for (; (start!=end)&&(wrap.pos!=wrapend); ++wrap.pos,++start) {
        if ((*start&0xc0)!=0x80) {
          wrapend=wrap.pos;
          break;
        }
        *wrap.pos=*start;
      }
      if ( (wrap.pos!=wrapend)&&(!endOfInput) ) {
        return true; // wait for more
      }
      if (!utf8segmenter<SegWrap&>(wrap,&wrap.leftover[0],wrap.pos)) {
        return false;
      }
      wrap.pos=0; // nullptr
    }
    if (!endOfInput) {
      wrap.curEnd=end;
    }
    return utf8segmenter<SegWrap&>(wrap,start,end);
  }
  // }}}

private:
  struct SegWrap {
    SegWrap(Segments seg) : seg(seg),pos(0) {}

    bool mustEscape(char ch) { return seg.mustEscape(ch); }
    void writeEscape(char ch) { seg.writeEscape(ch); }
    void write(const char *start,const char *end) { seg.write(start,end); }
    void writeUtf8(int ch,const char *start,const char *end) { seg.writeUtf8(ch,start,end); }

    bool error(const char *start,const char *end) { // {{{
      if (end==curEnd) {
        // assert(end); assert(!pos);
        if ( (end-start<4)&&((*start&0xc0)==0xc0) ) {
          pos=&leftover[0];
          for (; start!=end; ++pos,++start) {
            *pos=*start;
          }
        }
        return false;
      }
      return seg.error(start,end);
    }
    // }}}

    Segments seg;
    const char *curEnd;
    char leftover[4],*pos;
  };
  SegWrap wrap;
};

