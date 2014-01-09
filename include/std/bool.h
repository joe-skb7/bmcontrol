#ifndef STD__BOOL_H
#define STD__BOOL_H

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)

# include <stdbool.h>

#else

# ifdef __cplusplus
typedef bool _Bool;
# else
#  define _Bool signed char
# endif

# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1

#endif

#endif /* STD__BOOL_H */
