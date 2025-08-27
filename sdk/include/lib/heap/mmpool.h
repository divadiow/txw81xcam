#ifndef _HGIC_MMPOOL_H_
#define _HGIC_MMPOOL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "list.h"

#define MMPOOL_REGION_MAX (4)
struct mmpool {
    const char *name;
    uint32 size, free_size;
    uint8  region_cnt;
    uint8  align;
    uint8  headsize;
    uint8  tailsize;
    uint8  trace_off;
    uint8  ofchk_off;
    uint32 regions[MMPOOL_REGION_MAX][2];
    struct list_head free_list;
    struct list_head used_list;
#ifdef MMPOOL_PERF_COUNT
    uint32 tot_alloc, tot_free;
    uint32 alloc_perf[MMPOOL_PERF_COUNT];
    uint32 free_perf[MMPOOL_PERF_COUNT];
#endif
};

#define MMPOOL2_REGION_MAX (4)
#define MMPOOL2_FRAG_LOG (12)
struct mmpool2 {
    const char *name;
    uint32 size, free_size;
    uint8  region_cnt;
    uint8  align;
    uint8  headsize;
    uint8  tailsize;
    uint8  trace_off;
    uint8  ofchk_off;
    uint32 regions[MMPOOL2_REGION_MAX][2];
    struct list_head free_list[MMPOOL2_FRAG_LOG];
    struct list_head used_list;
#ifdef MMPOOL_PERF_COUNT
    uint32 tot_alloc, tot_free;
    uint32 alloc_perf[MMPOOL_PERF_COUNT];
    uint32 free_perf[MMPOOL_PERF_COUNT];
#endif
};

#define MMPOOL3_REGION_MAX (4)
#define MMPOOL3_FRAG_LOG  (12)
struct mmpool3_region {
    uint32 start, end;
    uint32 blk_size, blocks, blk_cnt; 
    struct list_head block_list;
};
struct mmpool3 {
    const char *name;
    uint32 size, free_size;
    uint8  region_cnt;
    uint8  align;
    uint8  headsize;
    uint8  tailsize;
    uint8  trace_off;
    uint8  ofchk_off;
    uint8  min_size;
    struct mmpool3_region regions[MMPOOL3_REGION_MAX];
    struct list_head used_list;
    struct list_head frag_list[MMPOOL3_FRAG_LOG];
#ifdef MMPOOL_PERF_COUNT
    uint32 tot_alloc, tot_free;
    uint32 alloc_perf[MMPOOL_PERF_COUNT];
    uint32 free_perf[MMPOOL_PERF_COUNT];
#endif
};

uint32_t cpu_cycle_diff(uint8_t sub, uint32_t last_cycle);
void *mmpool_alloc(struct mmpool *mp, uint32 size, const char *func, int32 line);
void mmpool_free(struct mmpool *mp, void *ptr);
int32 mmpool_init(struct mmpool *mp, uint32 addr, uint32 size);
int32 mmpool_free_state(struct mmpool *mp, uint32 *stat_buf, int32 size, uint32 *tot_size);
int32 mmpool_used_state(struct mmpool *mp, uint32 *stat_buf, int32 size, uint32 *tot_size, uint32 mini_size);
int32 mmpool_add_region(struct mmpool *mp, uint32 addr, uint32 size);
uint32 mmpool_free_size(struct mmpool *mp, uint32 min);
int32 mmpool_of_check(struct mmpool *mp, uint32 addr, uint32 size);
int32 mmpool_valid_addr(struct mmpool *mp, uint32 addr);
int32 mmpool_perfermance(struct mmpool *mp, uint8 alloc, uint32 *values, uint32 count);
int32 mmpool_used_list(struct mmpool *mp, uint32_t *list_buf, int32 list_size);
void mmpool_dump(struct mmpool *mp);
void mmpool_time(struct mmpool *mp, uint32 *alloc_time, uint32 *free_time);

void *mmpool2_alloc(struct mmpool2 *mp, uint32 size, const char *func, int32 line);
void mmpool2_free(struct mmpool2 *mp, void *ptr);
int32 mmpool2_init(struct mmpool2 *mp, uint32 addr, uint32 size);
int32 mmpool2_free_state(struct mmpool2 *mp, uint32 *stat_buf, int32 size, uint32 *tot_size);
int32 mmpool2_used_state(struct mmpool2 *mp, uint32 *stat_buf, int32 size, uint32 *tot_size, uint32 mini_size);
int32 mmpool2_add_region(struct mmpool2 *mp, uint32 addr, uint32 size);
uint32 mmpool2_free_size(struct mmpool2 *mp, uint32 min);
int32 mmpool2_of_check(struct mmpool2 *mp, uint32 addr, uint32 size);
int32 mmpool2_valid_addr(struct mmpool2 *mp, uint32 addr);
int32 mmpool2_perfermance(struct mmpool2 *mp, uint8 alloc, uint32 *values, uint32 count);
int32 mmpool2_used_list(struct mmpool2 *mp, uint32_t *list_buf, int32 list_size);
void mmpool2_dump(struct mmpool2 *mp);
void mmpool2_time(struct mmpool2 *mp, uint32 *alloc_time, uint32 *free_time);

void *mmpool3_alloc(struct mmpool3 *mp, uint32 size, const char *func, int32 line);
void mmpool3_free(struct mmpool3 *mp, void *ptr);
int32 mmpool3_add_region(struct mmpool3 *mp, uint32 addr, uint32 size, uint32 blk_size);
int32 mmpool3_init(struct mmpool3 *mp, uint32 addr, uint32 size, uint32 blk_size);
uint32 mmpool3_free_size(struct mmpool3 *mp, uint32 min);
int32 mmpool3_free_state(struct mmpool3 *mp, uint32 *stat_buf, int32 size, uint32 *tot_size);
int32 mmpool3_used_state(struct mmpool3 *mp, uint32 *stat_buf, int32 size, uint32 *tot_size, uint32 mini_size);
int32 mmpool3_of_check(struct mmpool3 *mp, uint32 addr, uint32 size);
int32 mmpool3_valid_addr(struct mmpool3 *mp, uint32 addr);
int32 mmpool3_perfermance(struct mmpool3 *mp, uint8 alloc, uint32 *values, uint32 count);
int32 mmpool3_used_list(struct mmpool3 *mp, uint32_t *list_buf, int32 list_size);
void mmpool3_dump(struct mmpool3 *mp);
void mmpool3_time(struct mmpool3 *mp, uint32 *alloc_time, uint32 *free_time);

#ifdef __cplusplus
}
#endif
#endif
