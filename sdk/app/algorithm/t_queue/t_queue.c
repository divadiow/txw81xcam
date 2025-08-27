/************************************************
 * 提供一个通用队列接口
************************************************/
#include "t_queue.h"
#include "osal/string.h"




void *tqueue_init()
{
    struct tqueue_base_s *tqueue_head = (struct tqueue_base_s *)os_malloc(sizeof(struct tqueue_base_s));
    if(tqueue_head)
    {
        tqueue_head->count = 0;
		INIT_LIST_HEAD((struct list_head*)tqueue_head);
    }

    return (void*)tqueue_head;
}

//正常需要先将列表清空才可以释放
void tqueue_deinit(void *tqueue_head_ptr)
{
    if(tqueue_head_ptr)
    {
        os_free(tqueue_head_ptr);
    }
}

//如果必要,暂时需要外部进行临界保护
//这里将data放到队列
uint8_t tqueue_push(void *tqueue_head_ptr,struct tqueue_s *data)
{
    struct tqueue_base_s *tqueue_head = (struct tqueue_base_s *)tqueue_head_ptr;
    struct tqueue_s *tqueue_data = data;
    if(tqueue_data)
    {
		list_add_tail((struct list_head*)tqueue_data,(struct list_head*)tqueue_head);
        tqueue_head->count++;
        return 0;
    }
    return 1;
}

//从队列取出一个data
struct tqueue_s *tqueue_pop(void *tqueue_head_ptr)
{
    struct tqueue_base_s *tqueue_head = (struct tqueue_base_s *)tqueue_head_ptr;
    struct tqueue_s *tqueue_data = NULL;
    if(tqueue_head->count > 0)
    {
		tqueue_data = (struct tqueue_s *)tqueue_head->next;
		list_del((struct list_head *)tqueue_data);
		INIT_LIST_HEAD((struct list_head *)tqueue_data);
        tqueue_head->count--;
        return tqueue_data;
    }
    else
    {
        return NULL;
    }
}


struct tqueue_s *tqueue_gen_data(void *data,tqueue_malloc m)
{
	struct tqueue_s *tqueue_data = (struct tqueue_s *)m(sizeof(struct tqueue_s));
	if(tqueue_data)
	{
		tqueue_data->data = data;
		INIT_LIST_HEAD((struct list_head *)tqueue_data);
	}
	return tqueue_data;
}