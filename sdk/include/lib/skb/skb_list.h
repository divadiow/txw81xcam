#ifndef __SKB_LIST_H__
#define __SKB_LIST_H__

#include "lib/skb/skb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct skb_list {
    struct sk_buff *next, *prev;
    uint32 count;
};

extern int32 skb_list_init(struct skb_list *list);
extern int32 skb_list_destroy(struct skb_list *list);
extern uint32 skb_list_count(struct skb_list *list);

#ifdef SKB_TRACE
#define skb_list_queue(list, skb)                _skb_list_queue(list, skb, __FUNCTION__, __LINE__)
#define skb_list_dequeue(list)                   _skb_list_dequeue(list, __FUNCTION__, __LINE__)
#define skb_list_first(list)                     _skb_list_first(list, __FUNCTION__, __LINE__)
#define skb_list_last(list)                      _skb_list_last(list, __FUNCTION__, __LINE__)
#define skb_list_unlink(skb, list)               _skb_list_unlink(skb, list, __FUNCTION__, __LINE__)
#define skb_list_queue_before(list, next, newsk) _skb_list_queue_before(list, next, newsk, __FUNCTION__, __LINE__)
#define skb_list_queue_after(list, prev, newsk)  _skb_list_queue_after(list, prev, newsk, __FUNCTION__, __LINE__)
#else
extern int32 skb_list_queue(struct skb_list *list, struct sk_buff *skb);
extern struct sk_buff *skb_list_dequeue(struct skb_list *list);
extern struct sk_buff *skb_list_first(struct skb_list *list);
extern struct sk_buff *skb_list_last(struct skb_list *list);
extern int32 skb_list_unlink(struct sk_buff *skb, struct skb_list *list);
extern int32 skb_list_queue_before(struct skb_list *list, struct sk_buff *next, struct sk_buff *newsk);
extern int32 skb_list_queue_after(struct skb_list *list, struct sk_buff *prev, struct sk_buff *newsk);
#endif

extern int32 skb_list_move(struct skb_list *to_list, struct skb_list *from_list);

extern int32 _skb_list_queue(struct skb_list *list, struct sk_buff *skb, char *func, int32 line);
extern struct sk_buff *_skb_list_dequeue(struct skb_list *list, char *func, int32 line);
extern struct sk_buff *_skb_list_first(struct skb_list *list, char *func, int32 line);
extern struct sk_buff *_skb_list_last(struct skb_list *list, char *func, int32 line);
extern int32 _skb_list_unlink(struct sk_buff *skb, struct skb_list *list, char *func, int32 line);
extern int32 _skb_list_queue_before(struct skb_list *list, struct sk_buff *next, struct sk_buff *newsk, char *func, int32 line);
extern int32 _skb_list_queue_after(struct skb_list *list,        struct sk_buff *prev, struct sk_buff *newsk, char *func, int32 line);

#ifdef __cplusplus
}
#endif

#endif

