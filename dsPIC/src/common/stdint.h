// <><><><><><><><><><><><><> stdint.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//	Copyright(C) 2012 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Basic types definitions
//
//-----------------------------------------------------------------------------

#ifndef __STDINT_H   // include only once
#define __STDINT_H

#ifndef __INTTYPES_DEFINED__

#ifndef int8_t
typedef	char				int8_t;
#define int8_t int8_t
#endif

#ifndef int_fast8_t
typedef	char				int_fast8_t;
#define int_fast8_t int_fast8_t
#endif

#ifndef uint8_t
typedef	unsigned char		uint8_t;
#define uint8_t uint8_t
#endif

#ifndef uint_fast8_t
typedef	unsigned char		uint_fast8_t;
#define uint_fast8_t uint_fast8_t
#endif

#ifndef int16_t
typedef short int			int16_t;
#define int16_t int16_t
#endif

#ifndef int_fast16_t
typedef short int			int_fast16_t;
#define int_fast16_t int_fast16_t
#endif

#ifndef uint16_t
typedef unsigned short int	uint16_t;
#define uint16_t uint16_t
#endif

#ifndef uint_fast16_t
typedef unsigned short int	uint_fast16_t;
#define uint_fast16_t uint_fast16_t
#endif

#ifndef int32_t
typedef long int			int32_t;
#define int32_t int32_t
#endif

#ifndef int_fast32_t
typedef long int			int_fast32_t;
#define int_fast32_t
#endif

#ifndef uint32_t
typedef unsigned long int	uint32_t;
#define uint32_t uint32_t
#endif

#ifndef uint_fast32_t
typedef unsigned long int	uint_fast32_t;
#define uint_fast32_t uint_fast32_t
#endif

#ifndef uint_fast32_t
typedef long long int		uint_fast32_t;
#define uint_fast32_t uint_fast32_t
#endif

#ifndef int_fast64_t
typedef long long int		int_fast64_t;
#define int_fast64_t int_fast64_t
#endif

#ifndef uint64_t
typedef unsigned long long int	uint64_t;
#define uint64_t uint64_t
#endif

#ifndef uint_fast64_t
typedef unsigned long long int	uint_fast64_t;
#define uint_fast64_t uint_fast64_t
#endif

#ifndef	__SIZE_TYPE__
#ifndef size_t 
typedef long unsigned int	size_t; // c++ ::new(size_t)
#define size_t size_t
#endif
#endif


#ifndef ssize_t
typedef long int		ssize_t;
#define ssize_t ssize_t
#endif

#ifndef fint
typedef int16_t			fint;
#define fint fint
#endif

#ifndef fuint
typedef	uint16_t		fuint;
#define fuint fuint
#endif

#endif	//	__INTTYPES_DEFINED__

#endif // __STDINT_H

// <><><><><><><><><><><><><> stdint.h <><><><><><><><><><><><><><><><><><><><><><>

