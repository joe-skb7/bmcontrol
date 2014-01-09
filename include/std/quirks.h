#ifndef STD__QUIRKS
#define STD__QUIRKS

/* "inline" isn't available at C89 standard */
#if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L)
# define inline
#endif

#endif /* STD__QUIRKS */
