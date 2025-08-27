#include "typesdef.h"
#include "fatfs/ff.h"
//#include "osal.h"
#include <string.h>
#include "osal_file.h"
#include "osal/string.h"


uint32_t file_mode (const char *mode)
{

  if (!mode) return 0;

  if (mode[0] == 'r') return FA_READ | FA_OPEN_EXISTING;
  if (mode[0] == 'w') return FA_READ | FA_WRITE | FA_CREATE_ALWAYS;
  if (mode[0] == 'a') return FA_READ | FA_WRITE | FA_CREATE_ALWAYS ;

  return 0;
}

F_FILE *osal_open(const char *filename, int oflags, int mode)
{
    uint32_t res;
	FIL* fp = os_malloc(sizeof(FIL));
	if(!fp){
		return 0;
	}    
	res = f_open (fp, filename, mode);
	if(res == FR_OK){
		return fp;
	}else{
		os_printf("%s res:%d\n",__FUNCTION__,res);
		os_free(fp);
		return 0;//return NULL;
	}
}

F_FILE *osal_fopen(const char *filename,const char *mode)
{
	return osal_open(filename, 0, file_mode(mode));
}

//返回值是读取到的字节数,与标准c返回值有一点点区别
uint32_t osal_fread(void *ptr,uint32_t size,uint32_t nmemb,F_FILE *fp)
{
	
#if WIN32 == 1
	return fread(ptr,size,nmemb,fp);
#else
	uint32_t readLen;
	uint32_t res = f_read(fp,ptr,size*nmemb,&readLen);
	if(res == FR_OK)
	{
		return readLen;
	}
	else
	{
		os_printf("%s res:%d\n",__FUNCTION__,res);
		return 0;
	}
#endif
} 


uint32_t osal_fwrite(void *ptr,uint32_t size,uint32_t nmemb,F_FILE *fp)
{
	
#if WIN32 == 1
	return fwrite(ptr,size,nmemb,fp);
#else
	uint32_t writeLen;
	uint32_t res =  f_write(fp,ptr,size*nmemb,&writeLen);
	if(res == FR_OK)
	{
		return writeLen;
	}
	else
	{
		return 0;
	}
#endif
	//return size*nmemb;
}

int osal_fclose(F_FILE *fp)
{
#if WIN32 == 1
	return fclose(fp);
#else
	int res =  f_close(fp);
	os_free(fp);
	return res;
#endif
}

uint32_t osal_ftell(F_FILE *fp)
{
#if WIN32 == 1
	return ftell(fp);
#else
	return f_tell(fp);
#endif
}

uint32_t osal_fseek(F_FILE *fp,uint32_t offset)
{
#if WIN32 == 1
	return fseek(fp,offset,SEEK_SET);
#else
	return f_lseek(fp,offset);
#endif
}

uint32_t osal_fsize(F_FILE *fp)
{
#if WIN32 == 1
		uint32_t tmp;
		uint32_t size;
		tmp = osal_ftell(fp);
		fseek(fp,0,SEEK_END);
		size = osal_ftell(fp);
		fseek(fp,tmp,SEEK_SET);
		return size;
#else
	return f_size(fp);
#endif
}




uint32_t osal_fread2 (char *buf, uint32_t is, uint32_t s, void *fp)
{
  uint32_t bw;
  FRESULT res;
	char *temp_buf = NULL;
	char *read_buf_locate = NULL;
	uint32_t buf_addr = (uint32_t)buf;

	uint32_t fp_tell = f_tell((FIL*)fp);
	uint8_t remain =0;
	
	//计算输入buf与未读扇区是否对齐
	if((buf_addr-fp_tell)%4)
	{
		remain = fp_tell%4;
	}
	
	//os_printf("fp_tell:%X\t%d\t%d\taddr:%X\terr:%d\r\n",fp_tell,remain,f_tell((FIL*)fp),buf_addr,(buf_addr-fp_tell)%4);
	if(remain || (buf_addr-fp_tell)%4)
	{
		temp_buf = os_malloc(is*s+4);	//申请一个缓冲buf,需要足够大小
		if(!temp_buf)
		{
			os_printf("osal_fread2 os_malloc err!\r\n");
			return 0;
		}
		read_buf_locate = temp_buf+remain;//remain是否4byte对齐,不对齐,代表需要填充4-remain才能4byte对齐,所以write_buf_locate长度应该为is*s+4-remain
																			//对应位置则为temp_buf+remain
	}
	else//不需要缓冲buf
	{
		read_buf_locate = buf;
	}
  res= f_read (fp, read_buf_locate, is*s, &bw);
  if (FR_OK != res) {
    // todo: errno
    //读取不成功也需要将缓冲buf释放
	if(read_buf_locate!=buf)
	{
		os_free(temp_buf);
	}

    os_printf("osal_fread err:%d\r\n!!!\r\n",res);
    return 0;
  }
	else//读取成功的话,判断是否为同一个buf,相同,代表没有申请新的缓冲buf
	{
		if(read_buf_locate!=buf)
		{
			memcpy(buf,read_buf_locate,bw);
			os_free(temp_buf);
		}

	}

  return bw;
}


uint32_t osal_fwrite2 (const char *buf, uint32_t is, uint32_t s, void *fp)
{
	uint32_t bw;
	FRESULT res;

	char *temp_buf = NULL;
	char *write_buf_locate = NULL;
	uint8_t shift = 0;	//提前写入字节数
	uint8_t remain =0;
	uint32_t buf_addr = (uint32_t)buf;

	uint32_t fp_tell = f_tell((FIL*)fp);

	//计算输入buf与未读扇区是否对齐
	if((buf_addr-fp_tell)%4)
	{
		remain = fp_tell%4;//需要填充多少byte
	}				
	
	if(remain || (buf_addr-fp_tell)%4)	//如果需要填充
	{
		if(remain)
		{
			shift = 4-remain;
		}
		//将多余的byte先写入
		res = f_write (fp, buf, shift, &bw);
		if (FR_OK != res) {
			// todo: errno
			//os_printf("1osal_fwrite2 result %d\r\n",res);
//			os_printf("1fp addr =  %d\r\n",fp);
			return 0;
		}

		temp_buf = os_malloc(is*s);//写入不超过is*s
		//空间不足
		if(!temp_buf)
		{
			os_printf("osal_fwrite2 os_malloc err!\r\n");
			return remain;//已经写入的remain字节
		}
		write_buf_locate = temp_buf;
		memcpy(write_buf_locate,buf+shift,is*s-shift);
	}
	else
	{
		write_buf_locate = (char*)buf;
	}

	res = f_write (fp, write_buf_locate, is*s-shift, &bw);
	bw += shift;//写入的总字节数


//无论写不写入,如果申请量的buf,都要释放
	if(temp_buf)
	{
		os_free(temp_buf);
	}

	
	if (FR_OK != res) {
	  // todo: errno
	 // os_printf("osal_fwrite2 result %d\r\n",res);
	 // os_printf("fp addr =  %d\r\n",fp);
	  return 0;
	}

  if(bw == 0)
  {
  		os_printf("bw = %d\rwantsize = %d\r\n",bw,is*s);
  }

  return bw;
}


char *osal_getcwd (char *buf, uint32_t len)
{
  if (FR_OK != f_getcwd (buf, len)) return NULL;
  return buf;
}

int osal_chdir (char *buf)
{
	int res;
  if (buf[1] == ':') {
    f_chdrive(buf);
    if (FR_OK != f_chdir(buf+2)) return -1;
  } else
  	{
  	 res = f_chdir(buf);
    if (FR_OK != res) 
		{
			os_printf("buf:%s\tres:%d\r\n",buf,res);
			return -1;
    	}
  	}
  return 0;
}


struct diriter {
  DIR     dir;
  FILINFO fil;
};

void *osal_opendir (char *path)
{
  struct diriter *dir;
  dir = os_malloc( sizeof(struct diriter) );
  if (!dir) return NULL;

  if( f_opendir( &dir->dir, path ) == FR_OK)
    return dir;

  os_free( dir );
  return NULL;
}

void osal_closedir (void *HDIR)
{
  struct diriter *dir = (struct diriter *)HDIR;
  if( !dir ) return;

  os_free( dir );
}

void *osal_readdir( void *HDIR )
{
  struct diriter *dir = (struct diriter *)HDIR;
  if( !dir ) return NULL;

  if( f_readdir( &dir->dir, &dir->fil ) != FR_OK )
    return NULL;

  if( dir->dir.sect == 0 )
    return NULL;
  return &dir->fil;
}

char *osal_dirent_name ( void *HFIL)
{
  FILINFO *fil = (FILINFO *)HFIL;
  if( !fil ) return NULL;
  return fil->fname;
}

int osal_dirent_isdir ( void *HFIL )
{
  FILINFO *fil = (FILINFO *)HFIL;
  if( !fil ) return 0;
  return fil->fattrib & AM_DIR;
}

uint32_t osal_dirent_date ( void *HFIL )
{
  FILINFO *fil = (FILINFO *)HFIL;
  return fil->fdate;
}

uint32_t osal_dirent_size ( void *HFIL )
{
  FILINFO *fil = (FILINFO *)HFIL;
  return fil->fsize;
}

uint32_t osal_fmkdir(const char *dir)
{
   return f_mkdir(dir); 
}


FRESULT osal_stat(const TCHAR* path,FILINFO* fno)
{
    return f_stat(path,fno);
}

FRESULT osal_unlink(const TCHAR* path)
{
    return f_unlink (path);
}

FRESULT osal_rename(const TCHAR* oldpath,const TCHAR* newpath)
{
    return f_rename (oldpath,newpath); 
}


FRESULT osal_fchmod(const TCHAR* path,uint8 attr,uint8 mask)
{
	return f_chmod(path,attr,mask);
}

FRESULT osal_fstat(const TCHAR* path,FILINFO* fno)
{
    return f_stat(path,fno);
}


//判断是否存在某个文件
FRESULT osal_fexist (const char *name)
{
  FIL *fp;
	fp = osal_fopen(name,"rb");
	if(fp)
	{
		osal_fclose(fp);
		return 0;
	}

  return 1;
}








