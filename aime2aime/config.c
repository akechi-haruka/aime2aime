#include <windows.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include "aime2aime/util/dprintf.h"
#include "aime2aime/config.h"

void aime2aime_config_load(
        struct aime2aime_config *cfg,
        const char *filename)
{
    assert(cfg != NULL);
    assert(filename != NULL);

    cfg->port = GetPrivateProfileIntA("aime2aime", "port", 3, filename);
    cfg->high_baud = GetPrivateProfileIntA("aime2aime", "high_baud", 1, filename) != 0;
    cfg->poll_timeout = GetPrivateProfileIntA("aime2aime", "poll_timeout", 0, filename);
    cfg->use_custom_led_flash = GetPrivateProfileIntA("aime2aime", "use_custom_led_flash", 1, filename) != 0;
    cfg->ignore_radio_off = GetPrivateProfileIntA("aime2aime", "ignore_radio_off", 0, filename) != 0;
}
