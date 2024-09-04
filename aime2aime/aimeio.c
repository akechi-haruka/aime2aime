#include <windows.h>
#include <time.h>
#include "subprojects/segapi/api/api.h"
#include "aime.h"
#include "config.h"
#include "util/dprintf.h"
#include "util/dump.h"

#define MIN_API_VER 0x010101

static struct aime2aime_config cfg;

static bool is_busy_by_game = false;
static bool is_polling_for_card = false;
static bool terminate_by_unload = false;
static uint64_t last_poll = 0;

void aime_io_nfc_radio_off();

HRESULT aime_debug_print_versions() {
    const uint32_t MAX_LEN = 64;

    uint32_t len = MAX_LEN;
    char *str = malloc(len);
    memset(str, 0, len);

    HRESULT hr = aime_get_hw_version(str, &len);
    if (!SUCCEEDED(hr)) {
        goto end_aime_debug_print_versions;
    }

    dprintf("aime2aime: HW Version: %s\n", str);

    len = MAX_LEN;
    memset(str, 0, len);

    hr = aime_get_fw_version(str, &len);
    if (!SUCCEEDED(hr)) {
        goto end_aime_debug_print_versions;
    }

    dprintf("aime2aime: FW Version: %s\n", str);

    end_aime_debug_print_versions:
    free(str);
    return hr;
}

DWORD WINAPI aime_api_process_thread_func(__attribute__((unused)) LPVOID lpParam) {
    while (!terminate_by_unload) {

        if (!is_busy_by_game) {

            uint8_t *rgb = api_get_aime_rgb_and_clear();
            if (rgb != NULL) {
                aime_led_set(rgb[0], rgb[1], rgb[2]);
            }

            if (api_get_card_switch_state()){
                is_polling_for_card = api_get_card_reading_state_and_clear_switch_state();
                dprintf("aime2aime: set polling by game API (%d)\n", is_polling_for_card);
                aime_set_polling(is_polling_for_card);
            }

        } else if (cfg.poll_timeout > 0){

            if (GetTickCount() - last_poll > cfg.poll_timeout){
                dprintf("aime2aime: Poll timeout reached (%llu/%d)\n", GetTickCount() - last_poll, cfg.poll_timeout);
                last_poll = GetTickCount();
                is_busy_by_game = false;
                is_polling_for_card = false;
                api_block_card_reader(false);
                dprintf("aime2aime: set polling by timeout off\n");
                aime_set_polling(false);
            }

        }

        Sleep(10);

    }

    return 0;
}

uint16_t aime_io_get_api_version(void) {
    dprintf("aime2aime: aime_io_get_api_version\n");
    return 0x0200;
}

HRESULT aime_io_init(void) {
    dprintf("aime2aime: Initializing\n");

    aime2aime_config_load(&cfg, ".\\aime2aime.ini");

    HRESULT hr = aime_connect(cfg.port, cfg.high_baud ? 115200 : 38400, cfg.use_custom_led_flash);
    if (!SUCCEEDED(hr)) {
        dprintf("aime2aime: Aime reader initialization failed: %lx", hr);
        return hr;
    }

    hr = aime_debug_print_versions();
    if (!SUCCEEDED(hr)) {
        dprintf("aime2aime: Aime check reader failed: %lx", hr);
        return hr;
    }

    hr = aime_led_reset();
    if (!SUCCEEDED(hr)) {
        dprintf("aime2aime: Aime LED initialization failed: %lx", hr);
        return hr;
    }

    api_init(".\\aime2aime.ini");

    CreateThread(NULL, 0, aime_api_process_thread_func, NULL, 0, NULL);

    dprintf("aime2aime: Loaded\n");

    api_send(PACKET_20_PING, 0, NULL);
    api_block_card_reader(false);

    return S_OK;
}

/*
    Poll for IC cards in the vicinity.

    - unit_no: Always 0 as of the current API version

    Minimum API version: 0x0100
 */
HRESULT aime_io_nfc_poll(__attribute__((unused)) uint8_t unit_no) {
    last_poll = GetTickCount();
    return aime_poll();
}

/*
    Attempt to read out a classic Aime card ID

    - unit_no: Always 0 as of the current API version
    - luid: Pointer to a ten-byte buffer that will receive the ID
    - luid_size: Size of the buffer at *luid. Always 10.

    Returns:

    - S_OK if a classic Aime is present and was read successfully
    - S_FALSE if no classic Aime card is present (*luid will be ignored)
    - Any HRESULT error if an error occured.

    Minimum API version: 0x0100
*/
HRESULT aime_io_nfc_get_aime_id(
        __attribute__((unused)) uint8_t unit_no,
        uint8_t *luid,
        size_t luid_size) {
    if (aime_get_card_type() != CARD_TYPE_MIFARE) {
        return S_FALSE;
    }

    memcpy(luid, aime_get_card_id(), min(luid_size, aime_get_card_len()));

    api_send(PACKET_26_CARD_AIME, aime_get_card_len(), (uint8_t*)aime_get_card_id());

    return S_OK;
}

/*
    Attempt to read out a FeliCa card ID ("IDm"). The following are examples
    of FeliCa cards:

    - Amuse IC (which includes new-style Aime-branded cards, among others)
    - Smartphones with FeliCa NFC capability (uncommon outside Japan)
    - Various Japanese e-cash cards and train passes

    Parameters:

    - unit_no: Always 0 as of the current API version
    - IDm: Output parameter that will receive the card ID

    Returns:

    - S_OK if a FeliCa device is present and was read successfully
    - S_FALSE if no FeliCa device is present (*IDm will be ignored)
    - Any HRESULT error if an error occured.

    Minimum API version: 0x0100
*/
HRESULT aime_io_nfc_get_felica_id(__attribute__((unused)) uint8_t unit_no, uint64_t *IDm) {
    if (aime_get_card_type() != CARD_TYPE_FELICA) {
        return S_FALSE;
    }

    memcpy(IDm, aime_get_card_id(), min(aime_get_card_type(), 8));

    api_send(PACKET_25_CARD_FELICA, aime_get_card_type(), (uint8_t*)aime_get_card_id());

    return S_OK;
}

/*
    Change the color and brightness of the card reader's RGB lighting

    - unit_no: Always 0 as of the current API version
    - r, g, b: Primary color intensity, from 0 to 255 inclusive.

    Minimum API version: 0x0100
*/

void aime_io_led_set_color(__attribute__((unused)) uint8_t unit_no, uint8_t r, uint8_t g, uint8_t b) {
    HRESULT hr = aime_led_set(r, g, b);
    if (!SUCCEEDED(hr)) {
        dprintf("aime2aime: LED set failed: %lx", hr);
    }
}

/*
    Switches the NFC radio to be turned on.

    Minimum API version: 0x0200
*/
void aime_io_nfc_radio_on() {
    last_poll = GetTickCount();
    is_busy_by_game = true;
    api_block_card_reader(true);
    dprintf("aime2aime: set polling by game radio on\n");
    aime_set_polling(true);
}

/*
    Switches the NFC radio to be turned off.

    Minimum API version: 0x0200
*/
void aime_io_nfc_radio_off() {
    if (!cfg.ignore_radio_off) {
        last_poll = GetTickCount();
        is_busy_by_game = false;
        is_polling_for_card = false;
        api_block_card_reader(false);
        dprintf("aime2aime: set polling by game radio off\n");
        aime_set_polling(false);
    } else {
        dprintf("aime2aime: radio off is ignored\n");
    }
}

BOOL __attribute__((unused)) WINAPI DllMain(__attribute__((unused)) HMODULE mod, DWORD cause, __attribute__((unused)) void *ctx)
{

    if (cause != DLL_PROCESS_DETACH) {
        return TRUE;
    }

    if (api_get_version() < MIN_API_VER){
        dprintf("aime2aime: API dll is outdated! At least v.%x is required, DLL is v.%x", MIN_API_VER, api_get_version());
        return FALSE;
    }

    terminate_by_unload = true;

    aime_close();

    return TRUE;
}
