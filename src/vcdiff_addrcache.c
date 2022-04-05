#include "vcdiff/addrcache.h"
#include <string.h>

#ifdef ARRAY_LENGTH
#undef ARRAY_LENGTH
#endif
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

void vcdiff_addrcache_init (vcdiff_cache_t *cache) {
	memset(cache, 0x00, sizeof(vcdiff_cache_t));
}

void vcdiff_addrcache_update (vcdiff_cache_t *cache, size_t addr) {
	/* update near cache */
	cache->near[cache->next_slot] = addr;
	cache->next_slot = (cache->next_slot + 1) % ARRAY_SIZE(cache->near);

	/* udpate same cache */
	cache->same[addr % ARRAY_SIZE(cache->same)] = addr;
}

vcdiff_mode_t vcdiff_addrcache_get_mode (uint8_t mode) {
	if (mode == 0x00) {
		return VCDIFF_MODE_SELF;
	}

	if (mode == 0x01) {
		return VCDIFF_MODE_HERE;
	}

	mode -= 2;
	if (mode < VCDIFF_CACHE_NEAR_SIZE) {
		return VCDIFF_MODE_NEAR;
	}

	mode -= VCDIFF_CACHE_NEAR_SIZE;
	if (mode < VCDIFF_CACHE_SAME_SIZE) {
		return VCDIFF_MODE_SAME;
	}

	return VCDIFF_MODE_ERROR;
}
