#ifndef STD__C89_QUIRKS
#define STD__C89_QUIRKS

#if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
# define inline
#endif

#endif /* STD__C89_QUIRKS */
