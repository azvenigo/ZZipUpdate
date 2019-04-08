// //////////////////////////////////////////////////////////
// Crc32.h
// Copyright (c) 2011-2016 Stephan Brumme. All rights reserved.
// Slicing-by-16 contributed by Bulat Ziganshin
// Tableless bytewise CRC contributed by Hagai Gold
// see http://create.stephan-brumme.com/disclaimer.html
//

#include <stdint.h>
#include <stddef.h>

/// compute CRC32 (Slicing-by-16 algorithm)
uint32_t crc32_16bytes (const void* data, size_t length, uint32_t previousCrc32 = 0);
/// compute CRC32 (Slicing-by-16 algorithm, prefetch upcoming data blocks)
uint32_t crc32_16bytes_prefetch(const void* data, size_t length, uint32_t previousCrc32 = 0, size_t prefetchAhead = 256);
