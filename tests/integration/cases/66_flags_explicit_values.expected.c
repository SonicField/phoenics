extern int snprintf(char *, unsigned long, const char *, ...);
typedef unsigned int HwReg;
#define HwReg_Enable (0x01u)
#define HwReg_Ready (0x02u)
#define HwReg_Error (0x80u)
#define HwReg__ALL (HwReg_Enable | HwReg_Ready | HwReg_Error)
#define HwReg__COUNT 3
#define HwReg__MAX_STRING 19

static inline int HwReg_has(HwReg flags, HwReg flag) {
    return (flags & flag) == flag;
}

static inline HwReg HwReg_set(HwReg flags, HwReg flag) {
    return flags | flag;
}

static inline HwReg HwReg_clear(HwReg flags, HwReg flag) {
    return flags & ~flag;
}

static inline const char *HwReg_to_string(HwReg p, char *buf, unsigned long len) {
    buf[0] = '\0';
    unsigned long pos = 0;
    if (p & HwReg_Enable) { pos += snprintf(buf + pos, len - pos, "%sEnable", pos ? "|" : ""); }
    if (p & HwReg_Ready) { pos += snprintf(buf + pos, len - pos, "%sReady", pos ? "|" : ""); }
    if (p & HwReg_Error) { pos += snprintf(buf + pos, len - pos, "%sError", pos ? "|" : ""); }
    if (pos == 0) { snprintf(buf, len, "(none)"); }
    return buf;
}
#line 6
