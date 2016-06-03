
namespace JsonChars {
inline bool is_ws(int ch) {
  return ( (ch==' ')||(ch=='\r')||(ch=='\n')||(ch=='\t') );
}

inline bool is_escape(int ch) {
  return (ch=='"')||(ch=='/')||(ch=='\\')||
         (ch=='b')||(ch=='f')||(ch=='n')||(ch=='r')||(ch=='t')||(ch=='u');
}

inline bool is_digit(int ch) {
  return ( (ch>='0')&&(ch<='9') );
}

inline bool is_hex(int ch) {
  return ( (ch>='0')&&(ch<='9') )||
         ( (ch>='a')&&(ch<='f') )||
         ( (ch>='A')&&(ch<='F') );
}

inline bool is_control0(int ch) {
  return ( (ch>=0)&&(ch<=0x1f) )||
         (ch==0x7f);
}

inline bool is_control1(int ch) {
  return ( (ch>=0x80)&&(ch<=0x9f) );
}

// TODO?!
inline bool is_quotefree(int ch) {
  return (ch=='$')||(ch=='_')||
         ( (ch>='A')&&(ch<='Z') )||
         ( (ch>='a')&&(ch<='z') )||
         (ch>0x7f);   // NOTE: no numbers!
}

} // namespace JsonChars

