#include <Arduino.h>
#include <pico/multicore.h>
#include <string.h>

#include "user_main.h"

extern "C" {
uint8_t *frontframe = nullptr;
}

void setup()
{
    memset(framebuffer, 0, sizeof(framebuffer));
    video_init();
    multicore_launch_core1(render_core_entry);
}

void loop()
{
    tight_loop_contents();
}
