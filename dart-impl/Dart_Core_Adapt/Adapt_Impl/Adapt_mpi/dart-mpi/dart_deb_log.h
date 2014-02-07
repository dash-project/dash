#ifndef DART_DEB_LOG_H_INCLUDED
#define DART_DEB_LOG_H_INCLUDED
#ifdef ENABLE_DEBUG
#define debug_print(...) do { printf (##__VA_ARGS__); } while (0)
#else
#define debug_print(...)
#endif
#endif /* DART_DEB_LOG_H_INCLUDED*/
