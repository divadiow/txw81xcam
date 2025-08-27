#ifndef _APP_PLAYBACK_H_
#define _APP_PLAYBACK_H_

void jpeg_file_get(uint8* photo_name,uint32 reset, char* file);
void jpeg_photo_explain(uint8* photo_name, uint32 scale_w, uint32 scale_h);
void rec_playback_thread_init(uint8* rec_name);




#endif
