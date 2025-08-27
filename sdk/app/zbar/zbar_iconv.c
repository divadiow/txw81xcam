//#include "type.h"
#include "zbar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <iconv.h>

#define FALSE_ERR       ('Z' - 'A')
#define DIFF_VALUE  ('a' - 'A')

int zbar_strcasecmp(char *s1, char *s2)
{
    int ch1 = 0;
    int ch2 = 0;

    //参数判断
    if(NULL == s1 || NULL == s2){
        return FALSE_ERR;
    }

    //逐个字符查找比对，并记录差值
    do{
        if((ch1 = *((unsigned char *)s1++)) >= 'A' && ch1 <= 'Z'){
            *s1 += DIFF_VALUE;
        }
        if((ch2 = *((unsigned char *)s2++)) >= 'A' && ch2 <= 'Z'){
            *s2 += DIFF_VALUE;
        }
    }while(ch1 && ch1 == ch2);

    return ch1 - ch2;
}


iconv_t zbar_iconv_open(const char *tocode, const char *fromcode){
	int *cd;
	//
	cd = calloc(1, sizeof(16));
	cd[0] = 0;
	
//	printf("%s %s\r\n",__func__,fromcode);
	if( ! zbar_strcasecmp( (char *)fromcode, "ISO8859-1" ) ){
		cd[0] = 1;
	}

	if( ! zbar_strcasecmp( (char *)fromcode, "SJIS" ) ){
		cd[0] = 2;
	}

	if( ! zbar_strcasecmp( (char *)fromcode, "UTF-8" ) ){
		cd[0] = 3;
	}
	//printf("%s %d %d\r\n",__func__,__LINE__,cd[0]);
	return cd;	
}


size_t zbar_iconv(unsigned char* _cd, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft){
	unsigned char *inb;
	unsigned char *outb;
	unsigned int inlen,outlen;
	unsigned int  itk = 0;
	inb = (unsigned char *)*inbuf;
	outb = (unsigned char *)*outbuf;
	outlen = 0;
	inlen = *inbytesleft;
	//printf("%s %d %d\r\n",__func__,__LINE__,_cd[0]);
	if(_cd[0] == 1){
		for(itk = 0;itk < inlen;itk++){
			if(inb[itk] < 0x80){
				outb[outlen] = inb[itk];
				outlen++;
			}else{
				outb[outlen] = (inb[itk]>>6)|0xc0;
				outlen++;
				outb[outlen] = (inb[itk]&0x3f)|0x80;
				outlen++;
			}
		}

		*inbuf = *inbuf + inlen;
		*outbuf = *outbuf + outlen;
		*outbytesleft = *outbytesleft - outlen;
		*inbytesleft = 0;

		return 0;
	}else if(_cd[0] == 3){
		memcpy(outb,inb,inlen);
		//*outbytesleft = inlen;

		*inbuf = *inbuf + inlen;
		*outbuf = *outbuf + inlen;
		*inbytesleft = 0;
		*outbytesleft = *outbytesleft - inlen;
		return 0;
	}else{
		printf("no iconv\r\n");
		return -1;
	}

	//printf("%s %d\r\n",__func__,__LINE__);

}



int zbar_iconv_close(iconv_t _cd){
	//printf("%s %d\r\n",__func__,__LINE__);

	free(_cd);
	
	//printf("%s %d\r\n",__func__,__LINE__);
	return 0;
}



