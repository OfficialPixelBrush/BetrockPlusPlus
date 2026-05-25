/*
 * Copyright (c) 2026, Pixel Brush <pixelbrush.dev>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 * 
*/

#define CHUNK_HEIGHT 		    128
#define CHUNK_WIDTH  		    16
#define CHUNK_AREA			    CHUNK_WIDTH * CHUNK_WIDTH
#define CHUNK_VOLUME		    CHUNK_WIDTH * CHUNK_WIDTH * CHUNK_WIDTH

#define SUB_CHUNK_SIZE		    16
#define SUB_CHUNK_VOLUME	    SUB_CHUNK_SIZE*SUB_CHUNK_SIZE*SUB_CHUNK_SIZE

#define WATER_LEVEL 		    64
#define NETHER_LAVA_LEVEL 	    32
// NOTE: Notch just copy-pasted the code for the overworld generator when making the Nether,
// so some values got wrongfully duplicated in the process. 
#define NETHER_BIOME_LAVA_LEVEL 64 // Comes about due to a copy-paste error by notch

#define PLAYER_EYE_HEIGHT       1.62
