#ifndef __INCLUDE_STDARG_H__
#define __INCLUDE_STDARG_H__

typedef __builtin_va_list va_list;
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

#endif