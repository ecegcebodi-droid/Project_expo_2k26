#include "storage_manager.h"
#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"

static nvs_handle_t tl_nvs_handle;
static bool nvs_ready = false;

void storage_init(void)
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        err = nvs_flash_init();
    }

    if (err == ESP_OK)
    {
        err = nvs_open(
            "terralink",
            NVS_READWRITE,
            &tl_nvs_handle
        );
    }

    if (err == ESP_OK)
    {
        nvs_ready = true;
    }
}

void storage_load_sos(tl_persistent_sos_t *sos)
{
    if (!sos)
        return;

    memset(sos, 0, sizeof(*sos));

    if (!nvs_ready)
        return;

    uint8_t pending = 0;

    nvs_get_u8(
        tl_nvs_handle,
        "pending",
        &pending
    );

    sos->pending = (pending != 0);

    nvs_get_u32(
        tl_nvs_handle,
        "sos_seq",
        &sos->sequence
    );

    size_t size = sizeof(double);

    nvs_get_blob(
        tl_nvs_handle,
        "sos_lat",
        &sos->latitude,
        &size
    );

    size = sizeof(double);

    nvs_get_blob(
        tl_nvs_handle,
        "sos_lon",
        &sos->longitude,
        &size
    );

    nvs_get_u8(
        tl_nvs_handle,
        "gps_fix",
        &pending
    );

    sos->gps_fix = (pending != 0);
}

void storage_save_sos(const tl_persistent_sos_t *sos)
{
    if (!nvs_ready || !sos)
        return;

    nvs_set_u8(
        tl_nvs_handle,
        "pending",
        sos->pending ? 1 : 0
    );

    nvs_set_u32(
        tl_nvs_handle,
        "sos_seq",
        sos->sequence
    );

    nvs_set_blob(
        tl_nvs_handle,
        "sos_lat",
        &sos->latitude,
        sizeof(double)
    );

    nvs_set_blob(
        tl_nvs_handle,
        "sos_lon",
        &sos->longitude,
        sizeof(double)
    );

    nvs_set_u8(
        tl_nvs_handle,
        "gps_fix",
        sos->gps_fix ? 1 : 0
    );

    nvs_commit(tl_nvs_handle);
}

void storage_clear_sos(void)
{
    if (!nvs_ready)
        return;

    nvs_erase_key(tl_nvs_handle, "pending");
    nvs_erase_key(tl_nvs_handle, "sos_seq");
    nvs_erase_key(tl_nvs_handle, "sos_lat");
    nvs_erase_key(tl_nvs_handle, "sos_lon");
    nvs_erase_key(tl_nvs_handle, "gps_fix");

    nvs_commit(tl_nvs_handle);
}