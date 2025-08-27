#ifndef __T_QUEUE_H
#define __T_QUEUE_H
#include "sys_config.h"
#include "typesdef.h"
#include "list.h"
struct tqueue_s
{
	struct list_head *next, *prev;
    void *data;
};

struct tqueue_base_s
{
	struct list_head *next, *prev;
    uint32_t count;
};

typedef void *(*tqueue_malloc)(uint32_t size);
typedef void (*tqueue_free)(void *data);


void *tqueue_init();
void tqueue_deinit(void *tqueue_head_ptr);
uint8_t tqueue_push(void *tqueue_head_ptr,struct tqueue_s *data);
struct tqueue_s *tqueue_pop(void *tqueue_head_ptr);
struct tqueue_s *tqueue_gen_data(void *data,tqueue_malloc m);
#endif