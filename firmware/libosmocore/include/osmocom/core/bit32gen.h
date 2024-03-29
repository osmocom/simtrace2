/*
 * bit32gen.h
 *
 * Copyright (C) 2014  Max <max.suraev@fairwaves.co>
 *
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

/*! \brief load unaligned n-byte integer (little-endian encoding) into uint32_t
 *  \param[in] p Buffer where integer is stored
 *  \param[in] n Number of bytes stored in p
 *  \returns 32 bit unsigned integer
 */
static inline uint32_t osmo_load32le_ext(const void *p, uint8_t n)
{
	uint8_t i;
	uint32_t r = 0;
	const uint8_t *q = (uint8_t *)p;
	for(i = 0; i < n; r |= ((uint32_t)q[i] << (8 * i)), i++);
	return r;
}

/*! \brief load unaligned n-byte integer (big-endian encoding) into uint32_t
 *  \param[in] p Buffer where integer is stored
 *  \param[in] n Number of bytes stored in p
 *  \returns 32 bit unsigned integer
 */
static inline uint32_t osmo_load32be_ext(const void *p, uint8_t n)
{
	uint8_t i;
	uint32_t r = 0;
	const uint8_t *q = (uint8_t *)p;
	for(i = 0; i < n; r |= ((uint32_t)q[i] << (32 - 8* (1 + i))), i++);
	return r;
}


/*! \brief store unaligned n-byte integer (little-endian encoding) from uint32_t
 *  \param[in] x unsigned 32 bit integer
 *  \param[out] p Buffer to store integer
 *  \param[in] n Number of bytes to store
 */
static inline void osmo_store32le_ext(uint32_t x, void *p, uint8_t n)
{
	uint8_t i;
	uint8_t *q = (uint8_t *)p;
	for(i = 0; i < n; q[i] = (x >> i * 8) & 0xFF, i++);
}

/*! \brief store unaligned n-byte integer (big-endian encoding) from uint32_t
 *  \param[in] x unsigned 32 bit integer
 *  \param[out] p Buffer to store integer
 *  \param[in] n Number of bytes to store
 */
static inline void osmo_store32be_ext(uint32_t x, void *p, uint8_t n)
{
	uint8_t i;
	uint8_t *q = (uint8_t *)p;
	for(i = 0; i < n; q[i] = (x >> ((n - 1 - i) * 8)) & 0xFF, i++);
}


/* Convenience function for most-used cases */


/*! \brief load unaligned 32-bit integer (little-endian encoding) */
static inline uint32_t osmo_load32le(const void *p)
{
	return osmo_load32le_ext(p, 32 / 8);
}

/*! \brief load unaligned 32-bit integer (big-endian encoding) */
static inline uint32_t osmo_load32be(const void *p)
{
	return osmo_load32be_ext(p, 32 / 8);
}


/*! \brief store unaligned 32-bit integer (little-endian encoding) */
static inline void osmo_store32le(uint32_t x, void *p)
{
	osmo_store32le_ext(x, p, 32 / 8);
}

/*! \brief store unaligned 32-bit integer (big-endian encoding) */
static inline void osmo_store32be(uint32_t x, void *p)
{
	osmo_store32be_ext(x, p, 32 / 8);
}
