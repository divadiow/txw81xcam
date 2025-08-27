#ifndef __OS_STRING_H__
#define __OS_STRING_H__
#include "typesdef.h"

#ifdef CSKY_OS
#include "osal/csky/string.h"
#endif

#ifdef OHOS
#include "osal/ohos/string.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*from kernel*/
#define KERN_SOH        "\001"      /* ASCII Start Of Header */
#define KERN_TSOH       "\002"      /* ASCII Start Of Tick Header */
#define KERN_EMERG      KERN_SOH"0" /* system is unusable */
#define KERN_ALERT      KERN_SOH"1" /* action must be taken immediately */
#define KERN_CRIT       KERN_SOH"2" /* critical conditions */
#define KERN_ERR        KERN_SOH"3" /* error conditions */
#define KERN_WARNING    KERN_SOH"4" /* warning conditions */
#define KERN_NOTICE     KERN_SOH"5" /* normal but significant condition */
#define KERN_INFO       KERN_SOH"6" /* informational */
#define KERN_DEBUG      KERN_SOH"7" /* debug-level messages */

void print_level(int8 level);
void *_os_malloc(int size);
void _os_free(void *ptr);
void *_os_zalloc(int size);
void *_os_realloc(void *ptr, int size);
void *_os_calloc(size_t nmemb, size_t size);

void *_os_malloc_t(int size, const char *func, int line);
void _os_free_t(void *ptr);
void *_os_zalloc_t(int size, const char *func, int line);
void *_os_realloc_t(void *ptr, int size, const char *func, int line);
void *_os_calloc_t(size_t nmemb, size_t size, const char *func, int line);

void *_os_malloc_psram(int size);
void _os_free_psram(void *ptr);
void *_os_zalloc_psram(size_t size);
void *_os_realloc_psram(void *ptr, int size);
void *_os_calloc_psram(size_t nmemb, size_t size);

void *_os_malloc_psram_t(int size, const char *func, int line);
void _os_free_psram_t(void *ptr);
void *_os_zalloc_psram_t(int size, const char *func, int line);
void *_os_realloc_psram_t(void *ptr, int size, const char *func, int line);
void *_os_calloc_psram_t(size_t nmemb, size_t size, const char *func, int line);

void mem_free_rec(void *addr, void *lr);
void mem_alloc_rec(void *addr, void *lr);
void mem_reclist_dump(void);

#ifdef M2M_DMA
void hw_memcpy(void *dest, const void *src, uint32 size);
void hw_memset(void * dest, uint8 val, uint32 n);
void hw_memcpy0(void *dest, const void *src, uint32 size);
#else
#define hw_memcpy  os_memcpy
#define hw_memset  os_memset
#define hw_memcpy0 os_memcpy
#endif

void *_os_memcpy(void *str1, const void *str2, int32 n);
char *_os_strcpy(char *dest, const char *src);
void *_os_memset(void *str, int c, int32 n);
void *_os_memmove(void *str1, const void *str2, size_t n);
char *_os_strncpy(char *dest, const char *src, int32 n);
int _os_sprintf(char *str, const char *format, ...);
int _os_vsnprintf(char *s, size_t n, const char *format, va_list arg);
int _os_snprintf(char *str, size_t size, const char *format, ...);
int32 os_strtok(char *str, char *separator, char *argv[], int argv_size);

#ifndef MAC2STR
#define MAC2STR(a) (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#ifndef STR2MAC
#define STR2MAC(s, a) str2mac((char *)(s), (uint8 *)(a))
#endif

#ifndef IP2STR_H
#define IP2STR_H(ip) ((ip)>>24)&0xff,((ip)>>16)&0xff,((ip)>>8)&0xff,(ip&0xff)  //host byteorder
#define IP2STR_N(ip) (ip&0xff),((ip)>>8)&0xff,((ip)>>16)&0xff,((ip)>>24)&0xff  //network byteorder
#define IPSTR "%d.%d.%d.%d"
#endif

#define MAC_EQU(a1,a2)    (os_memcmp((a1), (a2), 6)==0)
#define IS_BCAST_ADDR(a)  (((a)[0]&(a)[1]&(a)[2]&(a)[3]&(a)[4]&(a)[5]) == 0xff)
#define IS_MCAST_ADDR(a)  ((a)[0]&0x01)
#define IS_ZERO_ADDR(a)   (!((a)[0] | (a)[1] | (a)[2] | (a)[3] | (a)[4] | (a)[5]))
#define SSID_MAX_LEN      (32)
#define PASSWD_MAX_LEN    (63)

#define os_printf(fmt, ...)  hgprintf(KERN_TSOH fmt, ##__VA_ARGS__)
#define _os_printf(fmt, ...) hgprintf(fmt, ##__VA_ARGS__)

void disable_print(int8 dis);
void disable_print_color(int8 dis);
void hgprintf(const char *fmt, ...);
void hgprintf_out(char *str, int32 len, uint8 level);

typedef void (*osprint_hook)(void *priv, char *msg);
void print_redirect(osprint_hook hook, void *priv);

int32 hexchr2int(char c);
int32 hex2char(char *hex);
int32 hex2bin(char *hex, uint8 *buf, uint32 len);
void str2mac(char *macstr, uint8 *mac);
uint32 str2ip(char *ipstr);
void dump_hex(char *str, uint8 *data, uint32 len, int32 newline);
void dump_memory(char *title, uint32 *data, uint32 len);
void dump_key(char *str, uint8 *key, uint32 len, uint32 sp);
void key_str(uint8 *key, uint32 key_len, uint8 *str_buf);
void *os_memdup(const void *ptr, uint32 len);
int32 os_random_bytes(uint8 *data, int32 len);

extern int snprintf(char *str, size_t size, const char *format, ...);
extern int sprintf(char *string, const char *format, ...);
extern int vsprintf(char *str, const char *format, va_list arg_ptr);
extern int vsnprintf(char *str, size_t length, const char *format, va_list arg_ptr);
extern int printf(const char *format, ...);

#ifdef ERRLOG_ENABLE
void sys_errlog_save(char *log, int32 len, uint8 level);
void sys_errlog_flush(uint32 p1, uint32 p2, uint32 p3);
void sys_errlog_init(int8 level, uint16 buf_size);
void sys_errlog_dump(void);
#else
#define sys_errlog_save(log, len, level)
#define sys_errlog_flush(p1, p2, p3)
#define sys_errlog_init(level, buf_size)
#define sys_errlog_dump()
#endif

#ifdef __cplusplus
}
#endif
#endif
