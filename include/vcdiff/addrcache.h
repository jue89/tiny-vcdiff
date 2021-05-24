#ifndef VCDIFF_ADDRCACHE_H
#define VCDIFF_ADDRCACHE_H

#include <stdint.h>
#include <stddef.h>

#define VCDIFF_CACHE_NEAR_SIZE 4
#define VCDIFF_CACHE_SAME_SIZE 3

typedef enum {
	VCDIFF_MODE_SELF,
	VCDIFF_MODE_HERE,
	VCDIFF_MODE_NEAR,
	VCDIFF_MODE_SAME,
	VCDIFF_MODE_ERROR
} vcdiff_mode_t;

typedef struct {
	uint32_t near[VCDIFF_CACHE_NEAR_SIZE];
	uint32_t same[VCDIFF_CACHE_SAME_SIZE * 256];
	size_t next_slot;
} vcdiff_cache_t;

void vcdiff_addrcache_init (vcdiff_cache_t *cache);

uint32_t vcdiff_addrcache_update (vcdiff_cache_t *cache, uint32_t addr);

vcdiff_mode_t vcdiff_addrcache_get_mode (uint8_t mode);

static inline uint32_t vcdiff_addrcache_decode_self (vcdiff_cache_t *cache, uint32_t addr) {
	vcdiff_addrcache_update(cache, addr);
	return addr;
}

static inline uint32_t vcdiff_addrcache_decode_here (vcdiff_cache_t *cache, uint32_t here, uint32_t addr) {
	addr = here - addr;
	vcdiff_addrcache_update(cache, addr);
	return addr;
}

static inline uint32_t vcdiff_addrcache_decode_near (vcdiff_cache_t *cache, uint8_t mode, uint32_t addr) {
	mode -= 2;
	addr = cache->near[mode] + addr;
	vcdiff_addrcache_update(cache, addr);
	return addr;
}

static inline uint32_t vcdiff_addrcache_decode_same (vcdiff_cache_t *cache, uint8_t mode, uint32_t addr) {
	mode -= 2 + VCDIFF_CACHE_NEAR_SIZE;
	addr = cache->same[mode * 256 + addr];
	vcdiff_addrcache_update(cache, addr);
	return addr;
}

#endif
