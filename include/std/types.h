#ifndef STD__TYPES_H
#define STD__TYPES_H

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
# include <stdint.h>
# include <inttypes.h>
#else
# ifndef __WORDSIZE
#  define __WORDSIZE 32
# endif
typedef unsigned char		uint8_t;
typedef unsigned short int	uint16_t;
typedef unsigned int		uint32_t;
# if __WORDSIZE == 64
typedef unsigned long int	uint64_t;
# else
__extension__
typedef unsigned long long int	uint64_t;
# endif
#endif

#endif /* STD__TYPES_H */
