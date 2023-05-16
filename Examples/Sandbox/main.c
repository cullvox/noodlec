#include <stdio.h>

#include "noodle.h"

const char* pContent =
"audio = { \
    weapons = [ \"sword\", \"bow\", \"staff\" ], \
	music = { \
		enabled = true, \
		volume = 0.400000, \
	} \
	soundEffects = { \
		enabled = true, \
		volume = 0.600000, \
	} \
	volume = 0.800000, \
} \
player = { \
	inventory = { \
		potions = { \
			health = 5, \
			mana = 3, \
		} \
	} \
	level = 10, \
	name = \"John Doe\", \
} \
render = { \
	physicalDevice = 2, \
	resolution = { \
		height = 1080, \
		width = 1920, \
	} \
	shadowsEnabled = true, \
	textureQuality = \"high\", \
} \
window = { \
	fullscreen = true, \
	height = 1600, \
	monitor = 1, \
	someValue = [ 20, 40, 60, 80 ], \
	title = \"My Game Window\", \
	width = 1580, \
}";

int main(int argc, const char* argv[])
{
    char pErrorBuffer[256] = {0};

    noodleParse(pContent, pErrorBuffer, sizeof(pErrorBuffer) / sizeof(pErrorBuffer[0]));

	printf("%s\n", pContent);
}