#include "common.h"

#if defined(MBEDTLS_SHA256_C) || defined(MBEDTLS_SHA224_C)

#if defined(MBEDTLS_SHA256_ALT)

#include <string.h>
#include "sha256_alt.h"
#include "mbedtls/sha256.h"
#include "mbedtls/error.h"
#include "mbedtls/platform.h"


#include "osal/task.h"
#include "osal/mutex.h"


#define SHA256_HW_VERSION 1

struct os_mutex mbedtls_lock;

void mbedtls_sha256_mod_init()
{
    os_mutex_init(&mbedtls_lock);
}


void mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
    // os_printf("%s  CTX: 0x%x  task:0x%x\r\n", __FUNCTION__, ctx, os_task_current());
    struct sha_pack_mgr *mgr = os_malloc_psram(sizeof(struct sha_pack_mgr));
    memset(mgr, 0, sizeof(struct sha_pack_mgr));
    memset(ctx, 0, sizeof(struct mbedtls_sha256_context));

    ctx->hw = (struct sha_dev *)dev_get(HG_SHA_DEVID);
    ctx->mgr = mgr;
    ctx->mgr->isexceed = 0;
    ctx->mgr->total_len    = 0;
    INIT_LIST_HEAD(&ctx->mgr->packs);
}

void mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
    // os_printf("%s  CTX: 0x%x  task:0x%x\r\n", __FUNCTION__, ctx, os_task_current());
    struct sha_pack *pack, *n;
    list_for_each_entry_safe(pack, n, &ctx->mgr->packs, list) {
        if(*pack->refs > 0) {
            (*pack->refs)--;
        }
        if((*pack->refs) == 0) {
            if(pack->buf) {
                // printf("free buf : 0x%x\r\n", pack->buf);
                os_free_psram(pack->buf);
            }
            os_free_psram(pack->refs);
        }
        list_del(&pack->list);
        // printf("free pack : 0x%x\r\n", pack);
		os_free_psram(pack);
    }


    if(ctx->mgr) {
        os_free_psram(ctx->mgr);
    } else {
        os_printf("mbed free error\r\n");
    }
    mbedtls_platform_zeroize(ctx, sizeof(mbedtls_sha256_context));
}

void mbedtls_sha256_clone(mbedtls_sha256_context *dst,
                          const mbedtls_sha256_context *src)
{
    // os_printf("%s  src: 0x%x dst: 0x%x task:0x%x\r\n", __FUNCTION__, src, dst, os_task_current());
    struct sha_pack *pack, *n;
    struct sha_pack *new_pack;
    struct sha_pack_mgr *mgr_tmp = dst->mgr;
    uint32_t test_cnt = 0;
    *dst = *src;
    dst->mgr = mgr_tmp;
    dst->mgr->total_len = src->mgr->total_len;
    // printf("clone start\r\n");
    list_for_each_entry_safe(pack, n, &dst->mgr->packs, list) {
        test_cnt++;
    }
    if(test_cnt) {
        os_printf("mbed clone error\r\n");
    }

    list_for_each_entry_safe(pack, n, &src->mgr->packs, list) {
        (*pack->refs)++;
        new_pack = os_malloc_psram(sizeof(struct sha_pack));
        *new_pack = *pack;
        list_add_tail(&new_pack->list, &dst->mgr->packs);
        // printf("clone:   \r\n");
        // printf("opack -> buf: 0x%x len: 0x%x\r\n", pack->buf, pack->len);
        // printf("npack -> buf: 0x%x len: 0x%x\r\n", new_pack->buf, new_pack->len);
    }
    // printf("end start\r\n");
    // printf("mgr[0x%x, 0x%x]\r\n", src->mgr, dst->mgr);
}

/*
 * SHA-256 context setup
 */
int mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224)
{
    // os_printf("%s  CTX: 0x%x  task:0x%x\r\n", __FUNCTION__, ctx, os_task_current());
    struct sha_pack *pack, *n;
    uint32_t cnt = 0;
    list_for_each_entry_safe(pack, n, &ctx->mgr->packs, list) {
        cnt++;
    }
    if(cnt || (ctx->mgr == NULL)) {
        os_printf("mbed starts error cnt: %d  mgr: 0x%x\r\n", cnt, ctx->mgr);
    }
    ctx->mgr->total_len    = 0;
    return 0;
}

/*
 * SHA-256 process buffer
 */
int mbedtls_sha256_update(mbedtls_sha256_context *ctx,
                          const unsigned char *input,
                          size_t ilen)
{
    // os_printf("%s  CTX: 0x%x  task:0x%x\r\n", __FUNCTION__, ctx, os_task_current());
    if (ilen == 0) {
        return 0;
    }
	struct sha_pack *new_pack = os_malloc_psram(sizeof(struct sha_pack));
	struct sha_pack *pack, *n;
    if(ctx->mgr->isexceed) {
        //调用软件
        //mbedtls_sha256_update_sw(ctx, input, ilen);
    } else if(0) {
        //这里判断是否要转变为软件计算，比如说mgr->total_len + ilen > 某一数值
        ctx->mgr->isexceed = 1;
        list_for_each_entry_safe(pack, n, &ctx->mgr->packs, list) {
            //mbedtls_sha256_update_sw(ctx, pack->buf, pack->len);
            os_free_psram(pack->buf);
            list_del(&pack->list);
            os_free_psram(pack);
        }
        //mbedtls_sha256_update_sw(ctx, input, ilen);
    } else {
        //存下来，以求得在finish时调用硬件
        
        new_pack->buf = os_malloc_psram(ilen);
        new_pack->refs = os_malloc_psram(4);
        new_pack->len = ilen;
        *new_pack->refs = 1;
        ctx->mgr->total_len += ilen;
        memcpy(new_pack->buf, input, ilen);
		// printf("add tail buf: 0x%x len: 0x%x\r\n", new_pack->buf, new_pack->len);
        list_add_tail(&new_pack->list, &ctx->mgr->packs);
    }
    // return sha_update(ctx->hw, (uint8 *)input, (uint32)ilen);
    return 0;
}

static int fill_test(uint8_t *buf, uint8_t dat, uint32_t len)
{
    for(uint32_t i = 0;i<len;i++)
    {
        if(buf[i] != dat) {
            // printf("fill fail \r\n");
            return 1;
        }
    }
    return 0;
}

/*
 * SHA-256 final digest
 */
int mbedtls_sha256_finish(mbedtls_sha256_context *ctx,
                          unsigned char *output)
{
    // os_printf("%s  CTX: 0x%x  task:0x%x\r\n", __FUNCTION__, ctx, os_task_current());
    struct sha_pack *pack, *n;
    int ret = 0;
    unsigned int ac_len = 0;
    // printf("finish total len : %x\r\n", ctx->mgr->total_len);
    // ret = sha_final(ctx->hw, (uint8 *)output);
    // sha_ioctl(ctx->hw, SHA_IOCTL_CMD_END, 0);
	sha_ioctl(ctx->hw, SHA_IOCTL_CMD_START, 0);
    if(ctx->mgr->isexceed) {
        // mbedtls_sha256_finish_sw(ctx, output);
    } else {
        
        sha_ioctl(ctx->hw, SHA_IOCTL_CMD_RESET, 0);
        sha_init(ctx->hw, SHA_CALC_SHA256);
        list_for_each_entry_safe(pack, n, &ctx->mgr->packs, list) {
            // printf("update buf: 0x%x len: 0x%x\r\n", pack->buf, pack->len);
            sha_update(ctx->hw, pack->buf, pack->len);
            // if (fill_test(pack->buf, 'a', pack->len)) {
            //     printf("fill fail\r\n");
            // }
			// printf("input:\r\n");
            ac_len += pack->len;
			for(uint32_t i = 0;i<pack->len;i++)
			{
				// printf("%x", pack->buf[i]);
			}
			// printf("\r\n");
		}
    }
	// printf("ac total len : %x\r\n", ctx->mgr->total_len);
	// printf("update end\r\n");
    ret = sha_final(ctx->hw, (uint8 *)output);
    
    list_for_each_entry_safe(pack, n, &ctx->mgr->packs, list) {
        if(*pack->refs > 0) {
            (*pack->refs)--;
        }
        if((*pack->refs) == 0) {
            if(pack->buf) {
                // printf("free buf : 0x%x\r\n", pack->buf);
                os_free_psram(pack->buf);
            }
            os_free_psram(pack->refs);
        }
        list_del(&pack->list);
        // printf("free pack : 0x%x\r\n", pack);
		os_free_psram(pack);
    }
	sha_ioctl(ctx->hw, SHA_IOCTL_CMD_END, 0);
    // printf("free end\r\n");
    return ret;
}

void mbedtls_sha256_sp(unsigned char input,
                       unsigned int ilen,
                       unsigned char *output){
    struct sha_dev *hw = (struct sha_dev *)dev_get(HG_SHA_DEVID);
    uint8_t tmp[512];
    unsigned int uplen;
    memset(tmp, input, 512);
    sha_ioctl(hw, SHA_IOCTL_CMD_RESET, 0);
    sha_init(hw, SHA_CALC_SHA256);
    while(ilen)
    {
        uplen = (ilen > 512) ? 512 : ilen;
        sha_update(hw, tmp, uplen);
        ilen -= uplen;
    }
    sha_final(hw, (uint8 *)output);
}

#endif /* MBEDTLS_SHA256_ALT_H */

#endif /* MBEDTLS_SHA256_C || MBEDTLS_SHA224_C */