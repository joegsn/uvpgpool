//
//  byteorder_endian.h
//  UVPGPool
//
//  Created by Joseph Love on 3/19/14.
//  Contents are available elsewhere on the internet, and are Public Domain.
//

#ifndef __byteorder_endian_h__
#define __byteorder_endian_h__

#if defined(__linux__)
#  include <endian.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#elif defined(__APPLE__)
#  include <libkern/OSByteOrder.h>
#  define htobe16(x) OSSwapHostToBigInt16(x)
#  define htole16(x) OSSwapHostToLittleInt16(x)
#  define be16toh(x) OSSwapBigToHostInt16(x)
#  define le16toh(x) OSSwapLittleToHostInt16(x)

#  define htobe32(x) OSSwapHostToBigInt32(x)
#  define htole32(x) OSSwapHostToLittleInt32(x)
#  define be32toh(x) OSSwapBigToHostInt32(x)
#  define le32toh(x) OSSwapLittleToHostInt32(x)

#  define htobe64(x) OSSwapHostToBigInt64(x)
#  define htole64(x) OSSwapHostToLittleInt64(x)
#  define be64toh(x) OSSwapBigToHostInt64(x)
#  define le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(_MSC_VER)
#  include <winsock2.h>
#  define htobe16(x) htons(x)
#  define htobe32(x) htonl(x)
#  define htobe64(x) htonll(x)
#endif // __APPLE__

#endif // __byteorder_endian_h__
