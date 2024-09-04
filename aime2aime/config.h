#pragma once

#include <stddef.h>
#include <stdint.h>

#include <stdbool.h>

struct aime2aime_config {
    uint32_t port;
    bool high_baud;
    bool use_custom_led_flash;
    uint32_t poll_timeout;
    bool ignore_radio_off;
};

void aime2aime_config_load(
        struct aime2aime_config *cfg,
        const char *filename);
