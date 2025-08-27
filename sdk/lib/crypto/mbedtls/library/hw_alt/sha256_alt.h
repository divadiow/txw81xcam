#ifndef MBEDTLS_SHA256_ALT_H
#define MBEDTLS_SHA256_ALT_H

#if defined(MBEDTLS_SHA256_ALT)

#include "typesdef.h"
#include "list.h"
#include "dev.h"
#include "devid.h"

#include "mbedtls/private_access.h"
#include "mbedtls/build_info.h"
#include "mbedtls/platform_util.h"
#include "hal/sha.h"

struct sha_pack {
    struct list_head list;
    uint8_t *buf;
    uint32_t len;
    uint32_t *refs;
};

struct sha_pack_mgr {
    struct list_head packs;
    uint32_t isexceed;
    uint32_t total_len;
    uint32_t refs;
};

typedef struct mbedtls_sha256_context {
    struct sha_dev *MBEDTLS_PRIVATE(hw);
    struct sha_pack_mgr *mgr;
    uint32_t MBEDTLS_PRIVATE(total)[2];          /*!< The number of Bytes processed.  */
    uint32_t MBEDTLS_PRIVATE(state)[8];          /*!< The intermediate digest state.  */
    unsigned char MBEDTLS_PRIVATE(buffer)[64];   /*!< The data block being processed. */
    int MBEDTLS_PRIVATE(is224);                  /*!< Determines which function to use:
                                                    0: Use SHA-256, or 1: Use SHA-224. */
}
mbedtls_sha256_context;


#endif /* MBEDTLS_SHA256_ALT */

#endif /* MBEDTLS_SHA256_ALT_H */