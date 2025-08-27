#include "minilzo.h"
#include "lvImg.h"
#include "custom_mem.h"
#include "osal_file.h"
struct lvImg_res_gc_s *lvImg_gc_obj_add(struct lvImg_res_gc_s *gc_img,lv_img_dsc_t *img)
{
    struct lvImg_res_gc_s *head = (struct lvImg_res_gc_s *)custom_malloc(sizeof(struct lvImg_res_gc_s));
    head->img = img;
    
    if(gc_img)
    {
        head->next = gc_img;
    }
    else
    {
        head->next = NULL;
    }
    return head;
}

void fs_lv_img_free(lv_img_dsc_t * img)
{
    #ifdef PSRAM_HEAP
        custom_free_psram(img);
    #else
        custom_free(img);
    #endif
}

void lvImg_gc_loop(struct lvImg_res_gc_s *gc_img)
{
   struct lvImg_res_gc_s *head,*tmp_head;
   head = gc_img;
   while(head)
   {
        fs_lv_img_free(head->img);
        tmp_head = head;
        head = head->next;
        custom_free(tmp_head);
   } 
}

void *get_fs_lv_img(const char *filename)
{
    lv_img_dsc_t * img = NULL;
    void *fp = osal_fopen(filename,"rb");
    if(fp)
    {
        uint32_t filesize = osal_fsize(fp);
        #ifdef PSRAM_HEAP
            img = custom_malloc_psram(filesize);
        #else
            img = custom_malloc(filesize);
        #endif
        osal_fread(img,1,filesize,fp);
        osal_fclose(fp);
        img->data = (const uint8_t *)((uint32_t)&img->data + 4);
    }
    return img;
}


 
struct minilzo_head
{
    uint32_t src_len;
    uint32_t dst_len;
};

void *get_fs_lv_img_lzo(const char *filename)
{
    lv_img_dsc_t * img = NULL;
    struct minilzo_head head;
    void *fp = osal_fopen(filename,"rb");
    if(fp)
    {
        uint32_t filesize = osal_fsize(fp);
        uint32_t img_struct_size = 0;
        osal_fread(&head,1,sizeof(struct minilzo_head),fp);
        os_printf("src_len:%d\tdst_len:%d\n",head.src_len,head.dst_len);
        if(head.src_len > 0 && head.dst_len > 0)
        {
            img_struct_size = head.src_len + sizeof(lv_img_dsc_t);
            #ifdef PSRAM_HEAP
                img = custom_malloc_psram(img_struct_size);
            #else
                img = custom_malloc(img_struct_size);
            #endif

            //申请压缩的代码长度,解压需要用到的数据
            #ifdef PSRAM_HEAP
                uint8_t *img_lzo_buf = custom_malloc_psram(head.dst_len);
            #else
                uint8_t *img_lzo_buf = custom_malloc(head.dst_len);
            #endif
            //读取img的头数据
            osal_fread(img,1,sizeof(lv_img_dsc_t),fp);


            //读取lzo的数据,紧接的就是lzo数据
            osal_fread(img_lzo_buf,1,head.dst_len,fp);
            osal_fclose(fp);
            uint32_t new_len = head.src_len;
            //解压
            lzo1x_decompress_safe(img_lzo_buf,head.dst_len,((uint8_t*)img)+sizeof(lv_img_dsc_t),(lzo_uint*)&new_len,NULL);
            #ifdef PSRAM_HEAP
                custom_free_psram(img_lzo_buf);
            #else
                custom_free(img_lzo_buf);
            #endif
            //osal_fread(img,1,filesize-sizeof(struct minilzo_head),fp);
            img->data = (const uint8_t *)((uint8_t*)img)+sizeof(lv_img_dsc_t);
        }
        //非压缩
        else
        {
            #ifdef PSRAM_HEAP
                img = custom_malloc_psram(filesize-sizeof(lv_img_dsc_t));
            #else
                img = custom_malloc(filesize-sizeof(lv_img_dsc_t));
            #endif
            osal_fread(img,1,filesize-sizeof(lv_img_dsc_t),fp);
            osal_fclose(fp);
            img->data = (const uint8_t *)((uint32_t)&img->data + 4);
        }

    }
    return img;
}
