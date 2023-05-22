#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

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

	printf("Basic Noodle Parser Example written in C!\n");

  	NoodleGroup_t* pConfig = noodleParse(pContent, pErrorBuffer, sizeof(pErrorBuffer) / sizeof(pErrorBuffer[0]));
		if (!pConfig) 
	{
		printf("%s\n", pErrorBuffer);
		return EXIT_FAILURE;
	}

	NoodleGroup_t* pWindowConfig = noodleGroupFrom(pConfig, "window");
	assert(pWindowConfig);

	int monitor = noodleIntFrom(pWindowConfig, "monitor", NULL);
	const NoodleArray_t* pSomeValue = noodleArrayFrom(pWindowConfig, "someValue");
	assert(pSomeValue);

	int val_1 = noodleIntAt(pSomeValue, 1);

	noodleCleanup(pConfig);
	return EXIT_SUCCESS;
}