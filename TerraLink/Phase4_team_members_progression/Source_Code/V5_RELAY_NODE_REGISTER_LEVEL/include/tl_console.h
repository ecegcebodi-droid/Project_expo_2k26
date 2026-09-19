#ifndef TL_CONSOLE_H
#define TL_CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

void tl_console_print(const char *text);
void tl_console_println(const char *text);
void tl_console_u32(unsigned long value);
void tl_console_i32(long value);
void tl_console_line(const char *tag, const char *text);

#ifdef __cplusplus
}
#endif

#endif
