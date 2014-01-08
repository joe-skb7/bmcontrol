#ifndef C89_QUIRKS
#define C89_QUIRKS

#if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
# define inline
#endif

#endif /* C89_QUIRKS */
