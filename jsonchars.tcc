
namespace JsonChars {
inline bool is_ws(int ch) {
  return ( (ch==' ')||(ch=='\r')||(ch=='\n')||(ch=='\t') );
}

inline bool is_escape(int ch) {
  return (ch=='\\')||(ch=='"')||(ch=='\\')||(ch=='/')||
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

} // namespace JsonChars

