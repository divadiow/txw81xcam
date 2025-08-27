#include "osal/string.h"
#include "math.h"
#include "typesdef.h"
#include "sys_config.h"
#include "lib/vef/video_ef.h"
// #include "hal/dma.h"


	volatile vf_cblk g_vf_cblk;

#if 0
void memcpy16(uint32 dst, uint32 src, uint32 cnt)
{
	uint16* ps = src & 0xfffffffe;
	uint16* pd = dst & 0xfffffffe;
	for(uint32 i = 0;i<cnt/2;i++)
	{
		pd[i] = ps[i];
	}
}
#endif
void memcpy32(uint32 dst, uint32 src, uint32 cnt)
{
	// os_printf("dst : %x ,src : %x  ", dst, src);
	uint32* ps = (uint32*)(src & 0xfffffffc);
	uint32* pd = (uint32*)(dst & 0xfffffffc);
	// os_printf("pd : %x ,ps : %x\r\n", pd, ps);
	for(uint32 i = 0;i<cnt/4;i++)
	{
		pd[i] = ps[i];
	}
}

void line_inv(uint8 *in, uint8* out, uint8 *tmp,uint32 cnt)
{
#if 0
    os_memcpy(&tmp[cnt], in, cnt);
    for(uint32 i = 0;i<cnt;i++)
    {
        tmp[i] = tmp[2*cnt-1-i];
    }
    os_memcpy(out, tmp, cnt);
#elif 1
	uint32 *in32 = (uint32*)in;
	uint32 *out32 = (uint32*)out;
    for(uint32 i = 0;i<cnt/4;i++)
    {
        out32[cnt/4-i-1] = __builtin_bswap32 (in32[i]);
    }
#elif 0
	uint32 *in32 = (uint32*)in;
	uint32 *out32 = (uint32*)out;
    os_memcpy(&tmp[cnt], in, cnt);
    for(uint32 i = 0;i<cn/4;i++)
    {
        tmp[i] = tmp[2*cnt-1-i];
    }
    os_memcpy(out, tmp, cnt);
#elif 0
	uint16 *in16 = (uint16*)in;
	uint16 *out16 = (uint16*)out;
    for(uint32 i = 0;i<cnt/2;i++)
    {
        out16[cnt/4-i-1] = in16[i];
    }

#endif

}

void line_mov(uint8 *in, uint8* out, uint8 *tmp,uint32 cnt)
{
#if 0
    memcpy32(tmp, in, cnt);
    memcpy32(out, tmp, cnt);
#elif 1
	hw_memcpy(out, in, cnt);
#endif
}

void get_img_retc(uint8* iyuv, uint16 ph, uint16 pw, uint16 sh, uint16 sw,
				  uint8 *oyuv, uint16 oh, uint16 ow)
{
	uint16 s_row, s_col, e_row, e_col;
	uint16 col;
	uint16 img_col;
	uint8  *iy,*iu,*iv;
	uint8  *oy,*ou,*ov;

	s_row = sh;
	s_col = sw;
	e_row = s_row + oh;
	e_col = s_col + ow;
	iy = iyuv;
	iu = iy + ph*pw;
	iv = iu + ph*pw/4;
	oy = oyuv;
	ou = oy + oh*ow;
	ov = ou + oh*ow/4;
	uint8* line_tmp = os_malloc(ow);

	for(uint16 row = s_row, img_row=0;row<e_row;row++,img_row++) 
	{
		// for(uint16 col = s_col,img_col=0;col<e_col;col++,img_col++) {
		// 	oy[img_row*ow+img_col] = iy[row*pw+col];
		// 	ou[(img_row/2)*(ow/2)+img_col/2] = iu[(row/2)*(pw/2)+col/2];
		// 	ov[(img_row/2)*(ow/2)+img_col/2] = iv[(row/2)*(pw/2)+col/2];
		// }
        col = s_col;
        img_col = 0;
        hw_memcpy(&oy[img_row*ow+img_col], &iy[(row)*(pw)+col], e_col - s_col);
		// line_cpy(&iy[(row)*(pw)+col], &oy[img_row*ow+img_col], line_tmp, e_col - s_col);
        if( !(row & 1)) {
            memcpy(&ou[(img_row/2)*(ow/2)+img_col/2], &iu[(row/2)*(pw/2)+col/2], (e_col - s_col)/2);
            memcpy(&ov[(img_row/2)*(ow/2)+img_col/2], &iv[(row/2)*(pw/2)+col/2], (e_col - s_col)/2);
		// 	line_cpy(&ou[(img_row/2)*(ow/2)+img_col/2], &iu[(row/2)*(pw/2)+col/2], line_tmp, (e_col - s_col)/2);
		// 	line_cpy(&ov[(img_row/2)*(ow/2)+img_col/2], &iv[(row/2)*(pw/2)+col/2], line_tmp, (e_col - s_col)/2);
        }
	}
	os_free(line_tmp);
}

// extern __bobj struct dma_device *m2mdma;
void get_img_retc_blkcpy(uint8* iyuv, uint16 ph, uint16 pw, uint16 sh, uint16 sw,
				 		 uint8 *oyuv, uint16 oh, uint16 ow)
{
	uint8  *iy,*iu,*iv;
	uint8  *oy,*ou,*ov;
	iy = iyuv;
	iu = iy + ph*pw;
	iv = iu + ph*pw/4;
	oy = oyuv;
	ou = oy + oh*ow;
	ov = ou + oh*ow/4;
	// os_printf("%x %x\r\n", oy, &iy[sh*pw+sw]);
	// os_printf("bw %d  sw %d", ow, pw);
	// hw_blkcpy0(oy, &iy[sw] , ow, oh, pw, ow);
	// hw_blkcpy0(ou, &iu[(sh/2)*(pw/2)+(sw/2)] , ow/2, oh/2, pw/2, ow/2);
	// hw_blkcpy0(ov, &iv[(sh/2)*(pw/2)+(sw/2)] , ow/2, oh/2, pw/2, ow/2);

}


void put_img_retc(uint8* iyuv, uint16 ph, uint16 pw, 
				  uint8 *oyuv, uint16 sh, uint16 sw, uint16 oh, uint16 ow)
{
	uint16 s_row, s_col, e_row, e_col;
	uint16 col;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	uint16  img_col;
	s_row = sh;
	s_col = sw;
	e_row = s_row + ph;
	e_col = s_col + pw;
	iy = iyuv;
	iu = iyuv + ph*pw;
	iv = iyuv + ph*pw + ph*pw/4;
	oy = oyuv;
	ou = oyuv + oh*ow;
	ov = oyuv + oh*ow + oh*ow/4;

	for(uint16 row = s_row,img_row=0;row<e_row;row++,img_row++) 
	{
		// for(uint16 col = s_col,img_col=0;col<e_col;col++,img_col++) {
		// 	oy[row*ow+col] = iy[img_row*pw+img_col];

		// 	ou[(row/2)*(ow/2)+col/2] = iu[(img_row/2)*(pw/2)+img_col/2];
		// 	ov[(row/2)*(ow/2)+col/2] = iv[(img_row/2)*(pw/2)+img_col/2];
		// 	iindex++;
		// }
        col = s_col;
        img_col = 0;
        hw_memcpy(&oy[row*ow+col], &iy[(img_row)*(pw)+img_col], e_col - s_col);
        if( !(row & 1)) {
            memcpy(&ou[(row/2)*(ow/2)+col/2], &iu[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
            memcpy(&ov[(row/2)*(ow/2)+col/2], &iv[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
        }
	}
}

void put_img_retc_ix(uint8* iyuv, uint16 ph, uint16 pw, 
				  uint8 *oyuv, uint16 sh, uint16 sw, uint16 oh, uint16 ow)
{
	uint16 s_row, s_col, e_row, e_col;
	uint16 col;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	uint16 img_col;
	s_row = sh;
	s_col = sw;
	e_row = s_row + ph;
	e_col = s_col + pw;
	iy = iyuv;
	iu = iyuv + ph*pw;
	iv = iyuv + ph*pw + ph*pw/4;
	oy = oyuv;
	ou = oyuv + oh*ow;
	ov = oyuv + oh*ow + oh*ow/4;

    uint8* line_tmp = os_malloc(pw*2);

	for(uint16 row = s_row,img_row=0;row<e_row;row++,img_row++) 
	{
		// for(uint16 col = s_col,img_col=pw-1;col<e_col;col++,img_col--) {
		// 	oy[row*ow+col] = iy[img_row*pw+img_col];
		// 	ou[(row/2)*(ow/2)+col/2] = iu[(img_row/2)*(pw/2)+img_col/2];
		// 	ov[(row/2)*(ow/2)+col/2] = iv[(img_row/2)*(pw/2)+img_col/2];
		// 	iindex++;
		// }
        col = s_col;
        img_col = 0;
        line_inv(&iy[(img_row)*(pw)+img_col], &oy[row*ow+col], line_tmp, e_col - s_col);
        // hw_memcpy(&oy[row*ow+col], &iy[(img_row)*(pw)+img_col], e_col - s_col);
        if( !(row & 1)) {
            line_inv(&iu[(img_row/2)*(pw/2)+img_col/2], &ou[(row/2)*(ow/2)+col/2], line_tmp, (e_col - s_col)/2);
            line_inv(&iv[(img_row/2)*(pw/2)+img_col/2], &ov[(row/2)*(ow/2)+col/2], line_tmp, (e_col - s_col)/2);
            // hw_memcpy(&ou[(row/2)*(ow/2)+col/2], &iu[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
            // hw_memcpy(&ov[(row/2)*(ow/2)+col/2], &iv[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
        }
	}
    os_free(line_tmp);
}

void put_img_retc_iy(uint8* iyuv, uint16 ph, uint16 pw, 
				  uint8 *oyuv, uint16 sh, uint16 sw, uint16 oh, uint16 ow)
{
	uint16 s_row, s_col, e_row, e_col;
	uint16 col;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	uint16 img_col;
	s_row = sh;
	s_col = sw;
	e_row = s_row + ph;
	e_col = s_col + pw;
	iy = iyuv;
	iu = iyuv + ph*pw;
	iv = iyuv + ph*pw + ph*pw/4;
	oy = oyuv;
	ou = oyuv + oh*ow;
	ov = oyuv + oh*ow + oh*ow/4;

	for(uint16 row = s_row,img_row=ph-1;row<e_row;row++,img_row--) 
	{
		// for(uint16 col = s_col,img_col=0;col<e_col;col++,img_col++) {

		// 	oy[row*ow+col] = iy[img_row*pw+img_col];
		// 	ou[(row/2)*(ow/2)+col/2] = iu[(img_row/2)*(pw/2)+img_col/2];
		// 	ov[(row/2)*(ow/2)+col/2] = iv[(img_row/2)*(pw/2)+img_col/2];
		// 	iindex++;
		// }
        col = s_col;
        img_col = 0;
        hw_memcpy(&oy[row*ow+col], &iy[(img_row)*(pw)+img_col], e_col - s_col);
        if( !(row & 1)) {
            memcpy(&ou[(row/2)*(ow/2)+col/2], &iu[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
            memcpy(&ov[(row/2)*(ow/2)+col/2], &iv[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
        }
	}
}

void put_img_retc_ixy(uint8* iyuv, uint16 ph, uint16 pw, 
				  uint8 *oyuv, uint16 sh, uint16 sw, uint16 oh, uint16 ow)
{
	uint16 s_row, s_col, e_row, e_col;

	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	s_row = sh;
	s_col = sw;
	e_row = s_row + ph;
	e_col = s_col + pw;
	iy = iyuv;
	iu = iyuv + ph*pw;
	iv = iyuv + ph*pw + ph*pw/4;
	oy = oyuv;
	ou = oyuv + oh*ow;
	ov = oyuv + oh*ow + oh*ow/4;
	uint16 iindex = 0;
	for(uint16 row = s_row,img_row=ph-1;row<e_row;row++,img_row--) 
	{
		for(uint16 col = s_col,img_col=pw-1;col<e_col;col++,img_col--) {
			oy[row*ow+col] = iy[img_row*pw+img_col];
			ou[(row/2)*(ow/2)+col/2] = iu[(img_row/2)*(pw/2)+img_col/2];
			ov[(row/2)*(ow/2)+col/2] = iv[(img_row/2)*(pw/2)+img_col/2];
			iindex++;
		}
	}
}

void mov_img_retc(uint8* iyuv, uint16 ih, uint16 iw, uint16 imy, uint16 imx,
				  uint8 *oyuv, uint16 oh, uint16 ow, uint16 omy, uint16 omx, uint16 blkh, uint16 blkw)
{
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	iy = iyuv;
	iu = iyuv + ih*iw;
	iv = iyuv + ih*iw + ih*iw/4;
	oy = oyuv;
	ou = oyuv + oh*ow;
	ov = oyuv + oh*ow + oh*ow/4;
	uint8 line_tmp[640];

	// handle y data
	for(uint16 row = 0;row<blkh;row++)
	{
		line_mov(&iy[(imy+row)*(iw)+imx], &oy[(omy+row)*(ow)+omx], line_tmp, blkw);
		// hw_memcpy(&oy[(omy+row)*(ow)+omx], &iy[(imy+row)*(iw)+imx], blkw);
	}

	// // handle uv data
	for(uint16 row = 0;row<blkh/2;row++)
	{
		line_mov(&iu[(imy/2+row)*(iw/2)+imx/2], &ou[(omy/2+row)*(ow/2)+omx/2], line_tmp, blkw/2);
		// hw_memcpy(&ou[(omy/2+row)*(ow/2)+omx/2], &iu[(imy/2+row)*(iw/2)+imx/2], blkw/2);
	}

	for(uint16 row = 0;row<blkh/2;row++)
	{
		line_mov(&iv[(imy/2+row)*(iw/2)+imx/2], &ov[(omy/2+row)*(ow/2)+omx/2], line_tmp, blkw/2);
		// hw_memcpy(&ov[(omy/2+row)*(ow/2)+omx/2], &iv[(imy/2+row)*(iw/2)+imx/2], blkw/2);
	}
}

void mov_img_retc_ix(uint8* iyuv, uint16 ih, uint16 iw, uint16 imy, uint16 imx,
				  uint8 *oyuv, uint16 oh, uint16 ow, uint16 omy, uint16 omx, uint16 blkh, uint16 blkw)
{
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	iy = iyuv;
	iu = iyuv + ih*iw;
	iv = iyuv + ih*iw + ih*iw/4;
	oy = oyuv;
	ou = oyuv + oh*ow;
	ov = oyuv + oh*ow + oh*ow/4;
	uint8 line_tmp[640];

	// handle y data
	for(uint16 row = 0;row<blkh;row++)
	{
		line_inv(&iy[(imy+row)*(iw)+imx], &oy[(omy+row)*(ow)+omx], line_tmp, blkw);
	}

	// // handle uv data
	for(uint16 row = 0;row<blkh/2;row++)
	{
		line_inv(&iu[(imy/2+row)*(iw/2)+imx/2], &ou[(omy/2+row)*(ow/2)+omx/2], line_tmp, blkw/2);
	}

	for(uint16 row = 0;row<blkh/2;row++)
	{
		line_inv(&iv[(imy/2+row)*(iw/2)+imx/2], &ov[(omy/2+row)*(ow/2)+omx/2], line_tmp, blkw/2);
	}
}

void mov_img_retc_iy(uint8* iyuv, uint16 ih, uint16 iw, uint16 imy, uint16 imx,
				  uint8 *oyuv, uint16 oh, uint16 ow, uint16 omy, uint16 omx, uint16 blkh, uint16 blkw)
{
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	iy = iyuv;
	iu = iyuv + ih*iw;
	iv = iyuv + ih*iw + ih*iw/4;
	oy = oyuv;
	ou = oyuv + oh*ow;
	ov = oyuv + oh*ow + oh*ow/4;
	uint8 line_tmp[640];

	// handle y data
	for(uint16 row = 0;row<blkh;row++)
	{
		line_mov(&iy[(imy+row)*(iw)+imx], &oy[(omy+blkh-row)*(ow)+omx], line_tmp, blkw);
	}

	// // handle uv data
	for(uint16 row = 0;row<blkh/2;row++)
	{
		line_mov(&iu[(imy/2+row)*(iw/2)+imx/2], &ou[(omy/2+blkh/2-row)*(ow/2)+omx/2], line_tmp, blkw/2);
	}

	for(uint16 row = 0;row<blkh/2;row++)
	{
		line_mov(&iv[(imy/2+row)*(iw/2)+imx/2], &ov[(omy/2+blkh/2-row)*(ow/2)+omx/2], line_tmp, blkw/2);
	}
}

void mov_img_retc_iy1(uint8* iyuv, uint16 ih, uint16 iw, uint16 imy, uint16 imx,
				  uint8 *oyuv, uint16 oh, uint16 ow, uint16 omy, uint16 omx, uint16 blkh, uint16 blkw)
{
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	iy = iyuv;
	iu = iyuv + ih*iw;
	iv = iyuv + ih*iw + ih*iw/4;
	oy = oyuv;
	ou = oyuv + oh*ow;
	ov = oyuv + oh*ow + oh*ow/4;
	uint8 line_tmp[640];

	// handle y data
	for(uint16 row = 0;row<blkh;row++)
	{
		line_mov(&iy[(imy+row)*(iw)+imx], &oy[(omy+blkh-row)*(ow)+omx], line_tmp, blkw);
	}

	// // handle uv data
	for(uint16 row = 0;row<blkh/2;row++)
	{
		line_mov(&iu[(imy/2+row)*(iw/2)+imx/2], &ou[(omy/2+blkh/2-row)*(ow/2)+omx/2], line_tmp, blkw/2);
	}

	for(uint16 row = 0;row<blkh/2;row++)
	{
		line_mov(&iv[(imy/2+row)*(iw/2)+imx/2], &ov[(omy/2+blkh/2-row)*(ow/2)+omx/2], line_tmp, blkw/2);
	}
}

void put_img_hexagon(uint8* iyuv, uint16 ph, uint16 pw, double angle, 
                     uint8 *oyuv, int16 sh, int16 sw, uint16 oh, uint16 ow)
{
	int16 s_row, s_col, e_row, e_col;
	int16 row, col;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;
//	uint8 y_val, u_val, v_val;
	uint16 tri_h = (ph - pw)/2;

	uint32 strih;
	uint16 striw;
	uint16 stril;
	uint32 ucos = cos(angle/2)*524288;
	uint32 usin = sin(angle/2)*524288;
	volatile uint16 img_row = 0;
	volatile uint16 img_col = 0;
	s_row = sh;
	s_col = sw;
	e_row = s_row + ph;
	e_col = s_col + pw;
	iy = iyuv;
	iu = &iyuv[ph*pw];
	iv = &iyuv[ph*pw+ph*pw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];
	if(e_row > oh) {
		e_row = oh;
	}
	//os_printf("sr %d er %d\r\n", s_row, e_row);

    s_row = sh;
    // e_row = s_row + tri_h;
	for(row = s_row;row<e_row;row++,img_row++) 
	{
		if(row < 0) continue;
		strih = row - s_row;
		if(strih == 0) {
			s_col = sw+(pw/2);
			e_col = sw+(pw/2);
		} else if(strih < tri_h) {
			stril = strih *524288 / ucos;
			striw = stril * usin * 2 / 524288;
			//os_//os_printf("striw %d\r\n", striw);
			s_col = sw + (pw/2) - (striw/2);
			e_col = sw + (pw/2) + (striw/2);
		} else if(strih < tri_h+pw) {
			s_col = sw + (pw/2) - (pw/2);
			e_col = sw + (pw/2) + (pw/2);
		} else if(strih < tri_h+pw+tri_h) {
			strih = ph - strih - 1;
			stril = strih *524288 / ucos;
			striw = stril * usin * 2 / 524288;
			s_col = sw + (pw/2) - (striw/2);
			e_col = sw + (pw/2) + (striw/2);
		}

		img_col = s_col - sw;
		if(s_col <0) {
			img_col += 0 - (s_col);
			s_col = 0;
		} 
		if(e_col > ow) {
			e_col = ow;
		}
		// //os_printf("s %d e %d\r\n", s_col, e_col);
		//os_printf("row=%d irow=%d scol=%d ecol=%d\r\n", row, img_row, s_col, e_col);
		// for(col = s_col;col<e_col;col++,img_col++) {
		// 		oy[row*ow+col] = iy[(img_row)*(pw)+img_col];
		// }
        // if(row & 1) {
            // for(col = s_col;col<e_col;col++,img_col++) {
				// ou[(row/2)*(ow/2)+col/2] = iu[(img_row/2)*(pw/2)+img_col/2];
				// ov[(row/2)*(ow/2)+col/2] = iv[(img_row/2)*(pw/2)+img_col/2];
            // }
        // }
        col = s_col;
        hw_memcpy(&oy[row*ow+col], &iy[(img_row)*(pw)+img_col], e_col - s_col);
        if( !(row & 1)) {
            hw_memcpy(&ou[(row/2)*(ow/2)+col/2], &iu[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
            hw_memcpy(&ov[(row/2)*(ow/2)+col/2], &iv[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
        }

	}
    // s_row = e_row;
    // e_row = s_row + pw;
	// for(row = s_row;row<e_row;row++,img_row++) 
	// {
	// 	if(row < 0) continue;
	// 	strih = row - s_row;
	// 	if(strih == 0) {
	// 		s_col = sw+(pw/2);
	// 		e_col = sw+(pw/2);
	// 	} else if(strih < tri_h) {
	// 		stril = strih *524288 / ucos;
	// 		striw = stril * usin * 2 / 524288;
	// 		//os_//os_printf("striw %d\r\n", striw);
	// 		s_col = sw + (pw/2) - (striw/2);
	// 		e_col = sw + (pw/2) + (striw/2);
	// 	} else if(strih < tri_h+pw) {
	// 		s_col = sw + (pw/2) - (pw/2);
	// 		e_col = sw + (pw/2) + (pw/2);
	// 	} else if(strih < tri_h+pw+tri_h) {
	// 		strih = ph - strih - 1;
	// 		stril = strih *524288 / ucos;
	// 		striw = stril * usin * 2 / 524288;
	// 		s_col = sw + (pw/2) - (striw/2);
	// 		e_col = sw + (pw/2) + (striw/2);
	// 	}

	// 	img_col = s_col - sw;
	// 	if(s_col <0) {
	// 		img_col += 0 - (s_col);
	// 		s_col = 0;
	// 	} 
	// 	if(e_col > ow) {
	// 		e_col = ow;
	// 	}
	// 	// //os_printf("s %d e %d\r\n", s_col, e_col);
	// 	//os_printf("row=%d irow=%d scol=%d ecol=%d\r\n", row, img_row, s_col, e_col);
	// 	// for(col = s_col;col<e_col;col++,img_col++) {
	// 	// 		oy[row*ow+col] = iy[(img_row)*(pw)+img_col];
	// 	// }
    //     // if(row & 1) {
    //     //     for(col = s_col;col<e_col;col++,img_col++) {
	// 	// 		ou[(row/2)*(ow/2)+col/2] = iu[(img_row/2)*(pw/2)+img_col/2];
	// 	// 		ov[(row/2)*(ow/2)+col/2] = iv[(img_row/2)*(pw/2)+img_col/2];
    //     //     }
    //     // }
    //     col = s_col;
    //     hw_memcpy(&oy[row*ow+col], &iy[(img_row)*(pw)+img_col], e_col - s_col);
    //     if(row & 1) {
    //         hw_memcpy(&ou[(row/2)*(ow/2)+col/2], &iu[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
    //         hw_memcpy(&ov[(row/2)*(ow/2)+col/2], &iv[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
    //     }

	// }
    // s_row = e_row;
    // e_row = s_row + tri_h;
	// for(row = s_row;row<e_row;row++,img_row++) 
	// {
	// 	if(row < 0) continue;
	// 	strih = row - s_row;
	// 	if(strih == 0) {
	// 		s_col = sw+(pw/2);
	// 		e_col = sw+(pw/2);
	// 	} else if(strih < tri_h) {
	// 		stril = strih *524288 / ucos;
	// 		striw = stril * usin * 2 / 524288;
	// 		//os_//os_printf("striw %d\r\n", striw);
	// 		s_col = sw + (pw/2) - (striw/2);
	// 		e_col = sw + (pw/2) + (striw/2);
	// 	} else if(strih < tri_h+pw) {
	// 		s_col = sw + (pw/2) - (pw/2);
	// 		e_col = sw + (pw/2) + (pw/2);
	// 	} else if(strih < tri_h+pw+tri_h) {
	// 		strih = ph - strih - 1;
	// 		stril = strih *524288 / ucos;
	// 		striw = stril * usin * 2 / 524288;
	// 		s_col = sw + (pw/2) - (striw/2);
	// 		e_col = sw + (pw/2) + (striw/2);
	// 	}

	// 	img_col = s_col - sw;
	// 	if(s_col <0) {
	// 		img_col += 0 - (s_col);
	// 		s_col = 0;
	// 	} 
	// 	if(e_col > ow) {
	// 		e_col = ow;
	// 	}
	// 	// //os_printf("s %d e %d\r\n", s_col, e_col);
	// 	//os_printf("row=%d irow=%d scol=%d ecol=%d\r\n", row, img_row, s_col, e_col);
	// 	// for(col = s_col;col<e_col;col++,img_col++) {
	// 	// 		oy[row*ow+col] = iy[(img_row)*(pw)+img_col];
	// 	// }
    //     // if(row & 1) {
    //     //     for(col = s_col;col<e_col;col++,img_col++) {
	// 	// 		ou[(row/2)*(ow/2)+col/2] = iu[(img_row/2)*(pw/2)+img_col/2];
	// 	// 		ov[(row/2)*(ow/2)+col/2] = iv[(img_row/2)*(pw/2)+img_col/2];
    //     //     }
    //     // }
    //     col = s_col;
    //     hw_memcpy(&oy[row*ow+col], &iy[(img_row)*(pw)+img_col], e_col - s_col);
    //     if(row & 1) {
    //         hw_memcpy(&ou[(row/2)*(ow/2)+col/2], &iu[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
    //         hw_memcpy(&ov[(row/2)*(ow/2)+col/2], &iv[(img_row/2)*(pw/2)+img_col/2], (e_col - s_col)/2);
    //     }
	// }

}

void put_img_hexagon1(uint8* iyuv, uint16 ph, uint16 pw, double angle, uint16 retc_h,
                     uint8 *oyuv, int16 sh, int16 sw, uint16 oh, uint16 ow)
{
	int16 s_row, s_col = 0, e_row, e_col = 0;
	int16 row, col;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	uint16 tri_h = (ph - retc_h)/2;

	uint32 strih;
	uint32 striw;
	uint32 stril;
	uint32 ucos = cos(angle/2)*1048576;
	uint32 usin = sin(angle/2)*1048576;
	volatile uint16 img_row = 0;
	volatile uint16 img_col = 0;

	iy = iyuv;
	iu = &iyuv[ph*pw];
	iv = &iyuv[ph*pw+ph*pw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];

	s_row = sh;
	// e_row = s_row + tri_h;
	e_row = s_row + ph;
	if(s_row < 0) {
		img_row = 0 - s_row;
		s_row = 0;
	}
	if(e_row > oh) {
		e_row = oh;
	}
	//printf("srow %d erow %d  img_row %d\r\n", s_row, e_row, img_row);
	for(row = s_row;row<e_row;row++,img_row++) 
	{
		strih = row - sh;
		if(strih < tri_h) {
			stril = strih *1048576 / ucos;
			striw = stril * usin * 2 / 1048576;
			//printf("striw %d\r\n", striw);
			s_col = sw + (pw/2) - (striw/2);
			e_col = sw + (pw/2) + (striw/2);
		} else if(strih < tri_h+retc_h) {
			s_col = sw + (pw/2) - (pw/2);
			e_col = sw + (pw/2) + (pw/2);
			//continue;
		} else if(strih < tri_h+retc_h+tri_h) {
			strih = ph - strih - 1;
			stril = strih *1048576 / ucos;
			striw = stril * usin * 2 / 1048576;
			s_col = sw + (pw/2) - (striw/2);
			e_col = sw + (pw/2) + (striw/2);
		}

		img_col = s_col - sw;
		if(s_col <0) {
			img_col += 0 - (s_col);
			s_col = 0;
		} 
		if(e_col > ow) {
			e_col = ow;
		}

		col = s_col;
		//printf("s %d e %d row %d \r\n", s_col, e_col, row);
		// if(s_col > e_col) {
		// 	continue;
		// }
		//for(int16 col = s_col;col<e_col;col++,img_col++) {
			//if(col > 20-1 && col <25-1 && row > 40-1 && row <51-1){
				//oy[row*ow+col] = iy[(img_row)*(pw)+img_col];
				

		//}

		hw_memcpy(&oy[row*ow+col], &iy[(img_row)*(pw)+img_col], e_col-s_col);
		if(!(row&1)) {
			memcpy(&ou[(row/2)*(ow/2)+col/2], &iu[(img_row/2)*(pw/2)+img_col/2], (e_col-s_col)/2);
			memcpy(&ov[(row/2)*(ow/2)+col/2], &iv[(img_row/2)*(pw/2)+img_col/2], (e_col-s_col)/2);
		}
	}
	//printf("img_row %d\r\n", img_row);
}

void hexagon_table_init(uint8* iyuv, uint16 ph, uint16 pw, double angle, uint16 retc_h, uint16* htable)
{


	uint16 tri_h = (ph - retc_h)/2;

	uint32 strih;
	uint32 striw;
	uint32 stril;
	uint32 ucos = cos(angle/2)*1048576;
	uint32 usin = sin(angle/2)*1048576;



	//printf("srow %d erow %d  img_row %d\r\n", s_row, e_row, img_row);
	// for(row = s_row;row<e_row;row++,img_row++) 
	// {
	uint32 htmp;
	for(strih = 0;strih < tri_h+retc_h+tri_h;strih++)
	{
		htmp = strih;
		if(strih < tri_h) {
			stril = strih * 1048576 / ucos;
			striw = stril * usin * 2 / 1048576;
			//printf("striw %d\r\n", striw);
		} else if(strih < tri_h+retc_h) {
			striw = (pw);
			//continue;
		} else if(strih < tri_h+retc_h+tri_h) {
			htmp = ph - htmp - 1;
			stril = htmp *1048576 / ucos;
			striw = stril * usin * 2 / 1048576;
		}
		htable[strih] = striw;
	}
}


void put_img_hexagon2(uint8* iyuv, uint16 ph, uint16 pw, uint16* htable, uint16 retc_h,
                     uint8 *oyuv, int16 sh, int16 sw, uint16 oh, uint16 ow)
{
	int16 s_row, s_col, e_row, e_col;
	int16 row, col;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;



	uint32 strih;
	uint32 striw;

	volatile uint16 img_row = 0;
	volatile uint16 img_col = 0;

	iy = iyuv;
	iu = &iyuv[ph*pw];
	iv = &iyuv[ph*pw+ph*pw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];


	s_row = sh;
	// e_row = s_row + tri_h;
	e_row = s_row + ph;
	if(s_row < 0) {
		img_row = 0 - s_row;
		s_row = 0;
	}
	if(e_row > oh) {
		e_row = oh;
	}
	//printf("srow %d erow %d  img_row %d\r\n", s_row, e_row, img_row);
	for(row = s_row;row<e_row;row++,img_row++) 
	{
		strih = row - sh;
		striw = htable[strih];
		s_col = sw + (pw/2) - (striw/2);
		e_col = sw + (pw/2) + (striw/2);
		img_col = s_col - sw;
		if(s_col <0) {
			img_col += 0 - (s_col);
			s_col = 0;
		} 
		if(e_col > ow) {
			e_col = ow;
		}

		col = s_col;

		// printf("%x %x %x %x\r\n", s_col, e_col, col, img_col);
		//printf("s %d e %d row %d \r\n", s_col, e_col, row);
		// if(s_col > e_col) {
		// 	continue;
		// }
		//for(int16 col = s_col;col<e_col;col++,img_col++) {
			//if(col > 20-1 && col <25-1 && row > 40-1 && row <51-1){
				//oy[row*ow+col] = iy[(img_row)*(pw)+img_col];
		//}


		hw_memcpy(&oy[row*ow+col], &iy[(img_row)*(pw)+img_col], e_col-s_col);
		if(!(row&1)) {
			memcpy(&ou[(row/2)*(ow/2)+col/2], &iu[(img_row/2)*(pw/2)+img_col/2], (e_col-s_col)/2);
			memcpy(&ov[(row/2)*(ow/2)+col/2], &iv[(img_row/2)*(pw/2)+img_col/2], (e_col-s_col)/2);
		}
	}
	//printf("img_row %d\r\n", img_row);
}

void mov_img_hexagon(uint8* iyuv, uint16 ih, uint16 iw, int16 imy, int16 imx, 
                     uint8 *oyuv, uint16 oh, uint16 ow, int16 omy, int16 omx, 
					 uint16* htable, uint16 hag_h, uint16 hag_w)
{
	int16 s_row, s_col, e_row, e_col;
	int16 row, col;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;
	
	uint32 strih;
	uint16 striw;
	
	volatile uint16 img_row = 0;
	volatile uint16 img_col = 0;
	
	iy = iyuv;
	iu = &iyuv[ih*iw];
	iv = &iyuv[ih*iw+ih*iw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];


	s_row = omy;
	// printf("%d\r\n",omy);
	// e_row = s_row + tri_h;
	e_row = s_row + hag_h;
	if(s_row < 0) {
		img_row = 0 - s_row;
		s_row = 0;
	}
	if(e_row > oh) {
		e_row = oh;
	}
	// printf("srow %d erow %d  img_row %d\r\n", s_row, e_row, img_row);
	// printf("%d %d\r\n", sw, pw);
	for(row = s_row;row<e_row;row++,img_row++) 
	{
		strih = row - omy;
		// printf("strih %d \r\n", strih);
		striw = htable[strih];
		striw = striw & 0xfffc;
		// printf("striw %d strih %d\r\n", striw, strih);
		s_col = omx + (hag_w/2) - (striw/2);
		e_col = omx + (hag_w/2) + (striw/2);
		img_col = s_col - omx;
		if(s_col <0) {
			img_col += 0 - (s_col);
			s_col = 0;
		} 
		if(e_col > ow) {
			e_col = ow;
		}

		col = s_col;
		
		// printf("striw %d s %d e %d row %d \r\n", striw, s_col, e_col, row);
		// if(s_col > e_col) {
		// 	continue;
		// }
		//for(int16 col = s_col;col<e_col;col++,img_col++) {
			//if(col > 20-1 && col <25-1 && row > 40-1 && row <51-1){
				//oy[row*ow+col] = iy[(img_row)*(pw)+img_col];
		//}

		// memcpy(&oy[row*ow+col], &iy[(img_row)*(iw)+img_col], e_col-s_col);
		// if(!(row&1)) {
		// 	memcpy(&ou[(row/2)*(ow/2)+col/2], &iu[(img_row/2)*(iw/2)+img_col/2], (e_col-s_col)/2);
		// 	memcpy(&ov[(row/2)*(ow/2)+col/2], &iv[(img_row/2)*(iw/2)+img_col/2], (e_col-s_col)/2);
		// }

		// line_mov(&iy[(img_row + imy)*(iw)+(img_col+imx)], &oy[row*ow+col],  line_tmp, e_col-s_col);
		// if(!(row&1)) {
		// 	line_mov(&iu[((img_row+imy)/2)*(iw/2)+(img_col+imx)/2], &ou[(row/2)*(ow/2)+col/2], line_tmp, (e_col-s_col)/2);
		// 	line_mov(&iv[((img_row+imy)/2)*(iw/2)+(img_col+imx)/2], &ov[(row/2)*(ow/2)+col/2], line_tmp, (e_col-s_col)/2);
		// }

		memcpy32((uint32)&oy[row*ow+col], (uint32)&iy[(img_row + imy)*(iw)+(img_col+imx)], e_col-s_col);
		if(!(row&1)) {
			memcpy32((uint32)&ou[(row/2)*(ow/2)+col/2], (uint32)&iu[((img_row+imy)/2)*(iw/2)+(img_col+imx)/2], (e_col-s_col)/2);
			memcpy32((uint32)&ov[(row/2)*(ow/2)+col/2], (uint32)&iv[((img_row+imy)/2)*(iw/2)+(img_col+imx)/2], (e_col-s_col)/2);
		}
		
	}
	//printf("img_row %d\r\n", img_row);
}


void sw_scaler_shrink_x(uint8* iyuv, uint16 ph, uint16 pw,
              		  uint8 *oyuv, uint16 oh, uint16 ow)
{
	
	uint16 row;
	float col;
	uint16 irow,icol;
	uint16 orow,ocol;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;
	iy = iyuv;
	iu = &iyuv[ph*pw];
	iv = &iyuv[ph*pw+ph*pw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];
	// memset(ou, 128, oh*ow/4);
	// memset(ov, 128, oh*ow/4);
	float ih = (float)ph/(float)oh;
	float iw = (float)pw/(float)ow;
	printf("ih %f, iw %f\r\n", ih,iw);
	for(row = 0,orow = 0;row<ph;row++,orow++)
	{
		irow = row;
		for(col = 0,ocol=0;col<pw;col+=iw,ocol++) {
			icol = col;

			oy[orow*ow+ocol] = iy[irow*pw+icol];
		}

		if( !(row&1)) {
			for(col = 0,ocol=0;col<(pw/2);col+=iw,ocol++) {

				icol = col;
			// if(irow > 155 && icol > 20)
			//     printf("o(%d,%d) i(%d,%d) (%f)\r\n", orow,ocol, irow,icol, col);
				ov[(orow/2)*(ow/2)+ocol] = iv[(irow/2)*(pw/2)+icol];
				ou[(orow/2)*(ow/2)+ocol] = iu[(irow/2)*(pw/2)+icol];
			}
		}
	}
	// printf("row : %f , col : %f\r\n", row , col);
	// printf("orow : %f , ocol : %f", orow , ocol);
}

void sw_scaler_shrink1_x(uint8* iyuv, uint16 ph, uint16 pw,
              		  uint8 *oyuv, uint16 oh, uint16 ow)
{
	
	uint16 row;
	uint16 col;
	uint16 irow,icol;
	uint16 orow,ocol;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;
	uint8 index = 0;
	uint32 sum = 0;
	uint8 tmp = pw-ow;
	uint8 j_index[256];
	os_memset(j_index, 1, 256);

	iy = iyuv;
	iu = &iyuv[ph*pw];
	iv = &iyuv[ph*pw+ph*pw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];
	// memset(ou, 128, oh*ow/4);
	// memset(ov, 128, oh*ow/4);
	
	
	float f_index = (float)ow/(float)tmp;
	for(float fcol = 0;fcol<ow;fcol+=f_index)
	{	
		icol = fcol;
		j_index[icol]++;	
	}
	if(j_index[ow-1] == 2) {
		j_index[ow-1] = 1;
	}
	for(index = 0;index<ow;index++)
	{
		sum += j_index[index];
		// printf("%d ", j_index[index]);
	}
	// printf("\r\nsum %d \r\n",sum);
	// printf("ih %f, iw %f\r\n", ih,iw);
	for(row = 0,orow = 0;row<ph;row++,orow++)
	{
		index = 0;
		irow = row;
		for(col = 0,ocol=0;col<pw;col+=j_index[index++],ocol++) {
			icol = col;
			// printf("o(%d,%d) i(%d,%d) (%f)\r\n", orow,ocol, irow,icol, col);
			oy[orow*ow+ocol] = iy[irow*pw+icol];
		}
		index = 0;
		if( !(row&1)) {
			for(col = 0,ocol=0;col<(pw/2);col+=(j_index[index++]),ocol++) {
				icol = col;
			// if(irow > 155 && icol > 20)
			//     printf("o(%d,%d) i(%d,%d) (%d)\r\n", orow,ocol, irow,icol, col);
				ov[(orow/2)*(ow/2)+(ocol)] = iv[(irow/2)*(pw/2)+(icol)];
				ou[(orow/2)*(ow/2)+(ocol)] = iu[(irow/2)*(pw/2)+(icol)];
			}
		}
	}
	// printf("row : %f , col : %f\r\n", row , col);
	// printf("orow : %f , ocol : %f", orow , ocol);
}

void sw_line_shrink(uint8* iyuv, uint16 pw, uint8* oyuv, uint16 ow, uint8 *s_tmp)
{
	if(ow == 0){
		return;
	}
	uint32 jp1 = pw/ow;

	uint32 n2 = pw-(ow*jp1);
	

	memset(s_tmp, jp1, ow);
	// s_tmp[0] = jp1-1;
	// printf("iw %d , ow %d\r\n", pw, ow);
	// printf("n1 : %d , n2: %d\r\n", n1,n2);
	// printf(" n1 + n2 = %d\r\n", n1+n2);
	// printf(" n1*j1 + n2*j2 = %d\r\n",n1*jp1+n2*jp2);
	if(n2 == 0) {
		;
	} else {
		float f_i =  (float)ow/(float)n2;
		uint32 int_i = f_i*1024*512;
		for(;int_i<ow*1024*512;int_i+=f_i*1024*512)
		// for(;f_i < ow;f_i+=(float)ow/(float)n2)
		{
			s_tmp[(int_i)/1024/512]++;
			// s_tmp[(uint32)f_i]++;
		}
	}


	// test
	// uint32 sum = 0;
	// for(uint32 i = 0;i<ow;i++)
	// {
	// 	printf("%d ", s_tmp[i]);
	// 	sum += s_tmp[i];
	// }
	// printf("\r\nsum:%d\r\n", sum);

	uint32 iindex = 0;
	for(uint32 i = 0;i<ow;i++)
	{
		
		oyuv[i] = iyuv[iindex];
	 iindex += s_tmp[i];
	}

}

void sw_scaler_shrink2_x(uint8* iyuv, uint16 ph, uint16 pw,
              		  uint8 *oyuv, uint16 oh, uint16 ow)
{
	
	uint16 row;
	
	
	
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;
	
	
	
	uint8 temp[256];
	
	iy = iyuv;
	iu = &iyuv[ph*pw];
	iv = &iyuv[ph*pw+ph*pw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];

	for(row = 0;row<ph;row++)
	{
		sw_line_shrink(&iy[row*pw], pw, &oy[row*ow], ow, temp);
	}

	for(row = 0;row<ph/2;row++)
	{
		sw_line_shrink(&iu[row*(pw/2)], pw/2, &ou[row*ow/2], ow/2, temp);
		sw_line_shrink(&iv[row*(pw/2)], pw/2, &ov[row*ow/2], ow/2, temp);
	}
	// printf("row : %f , col : %f\r\n", row , col);
	// printf("orow : %f , ocol : %f", orow , ocol);
}

//leanar fill
void sw_y_fill_x(uint8* y1, uint8* y2, uint32 cnt)
{
	uint32 index = 0;
	int32 nval;
	float k = ( *(y2+1) - *(y2) ) / cnt;
	y1[0] = y2[0];
	while(cnt)
	{
		nval = y1[index] + (int8)k;
		if(nval > 255) {
			y1[index+1] = 255;
		} else if(nval < 0){
			y1[index+1] = 0;
		} else {
            y1[index+1] = nval;
        }
		index++;
		cnt--;
		k +=k;
		//printf("%d ",cnt);
	}
}	
//
void sw_uv_fill_x(uint8* uv1, uint8* uv2, uint32 cnt)
{
	uint32 index = 0;
	int32 nval;
	// cnt = cnt/2;
	float k = (*(uv2+1) - *(uv2)) / cnt;
	uv1[0] = uv2[0];
	while(cnt)
	{
		nval = uv1[index] + (int8)k;
		if(nval > 255) {
			uv1[index+1] = 255;
		} else if(nval < 0){
			uv1[index+1] = 0;
		} else {
            uv1[index+1] = nval;
        }
		index++;
		cnt--;
		k +=k;
	}
}

void sw_scaler_enlarge_x(uint8* iyuv, uint16 ph, uint16 pw,uint16 rn, uint16 cn,
              		  uint8 *oyuv, uint16 oh, uint16 ow)
{
	
	uint16 row, col;
	uint16 icol;
	uint16 orow,ocol;
	uint16 cnt;
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;

	if(cn&1) {
		ow = (cn+1)*pw;
	} else {
		ow = (pw-1)*cn+pw;
	}
    ow = (cn+1)*pw;
	iy = iyuv;
	iu = &iyuv[ph*pw];
	iv = &iyuv[ph*pw+ph*pw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];
	// memset(ou, 128, oh*ow/4);
	// memset(ov, 128, oh*ow/4);
	printf("----ow : %d\r\n", ow);
	for(row = 0,orow = 0;row<ph;row++,orow++)
	{
		for(col = 0,ocol = 0,cnt = 0;col<pw;col++,ocol++) {
			// oy[orow*ow+(ocol+cnt)] = iy[row*pw+col];
			// if(row < 30 && col < 20)
			//     printf("o(%d,%d) i(%d,%d)\r\n", orow,ocol, row,col);
			sw_y_fill_x(&oy[orow*ow+(ocol)], &iy[row*pw+col], cn);
			ocol+= cn;
		}

		if( !(row&1)) {
			for(col = 0,ocol=0;col<(pw/2);col++,ocol++) {
				icol = col;
				sw_uv_fill_x(&ou[(orow/2)*(ow/2)+ocol], &iu[(row/2)*(pw/2)+icol], cn);
				sw_uv_fill_x(&ov[(orow/2)*(ow/2)+ocol], &iv[(row/2)*(pw/2)+icol], cn);
				ocol+= cn;
			}
		}
	}
}

void sw_interpolt_1(uint8* v1, uint8* v2, uint32 cnt)
{
	uint32 index = 0;
	// cnt = cnt/2;
	v1[0] = v2[0];
	while(cnt)
	{
		v1[index+1] = v1[index];
		index++;
		cnt--;
	}
}

void sw_interpolt_2(uint8* v1, uint8* v2, uint32 cnt)
{
	uint32 index = 0;
	// cnt = cnt/2;
	
	int8 k = (*(v2+1) - *(v2)) / (cnt+3);
	v1[0] = v2[0];
	while(cnt)
	{
		// nval = v1[index] + k;
		// if(nval < 0) {
		// 	v1[index+1] = 0;
		// } else if(nval > 255) {
		// 	v1[index+1] = 255;
		// } else {
		// 	v1[index+1] = nval;
		// }
		v1[index+1] = v1[index] + k;
		index++;
		cnt--;
		k +=k;
	}
}

void sw_line_expend(uint8* iyuv, uint16 pw, uint8* oyuv, uint16 ow, uint8 *s_tmp)
{
	uint32 i1 = ow/pw;
	uint32 i2 = ow/pw-1;
	uint32 n1 = ow - (i1*pw);
	

	memset(s_tmp, i2, pw);
	// s_tmp[0] = i1-1;
	// printf("n1 : %d , n2: %d\r\n", n1,n2);
	// printf(" n1 + n2 = %d\r\n", n1+n2);
	// printf(" n1*i1 + n2*i2 = %d\r\n",n1*i1+n2*i2);
	if(n1 == 0) {
		;
	} else {
		double f_i =  (float)pw/(float)n1;
		uint32 int_i = f_i*1024*512;
		for(;int_i<pw*1024*512;int_i+=f_i*1024*512)
		// for(;f_i < ow;f_i+=(float)ow/(float)n2)
		{
			s_tmp[(int_i)/1024/512]++;
			// s_tmp[(uint32)f_i]++;
		}
	}

	// test
	// uint32 sum = pw;
	// for(uint32 i = 0;i<ow;i++)
	// {
	// 	printf("%d ", s_tmp[i]);
	// 	sum += s_tmp[i];
	// }
	// printf("\r\nsum:%d\r\n", sum);

	uint32 oindex = 0;
	for(uint32 i = 0;i<pw;i++)
	{
		sw_interpolt_2(&oyuv[oindex], &iyuv[i], s_tmp[i]);
		oindex += (s_tmp[i]+1);
	}

}

void sw_img_trans(uint8* iyuv, uint16 ph, uint16 pw,
              	   uint8 *oyuv, uint16 oh, uint16 ow)
{
	
	uint16 row;
	
	
	
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;
	
	
	
	uint8 temp[512];
	
	iy = iyuv;
	iu = &iyuv[ph*pw];
	iv = &iyuv[ph*pw+ph*pw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];
	uint32 b = pw/4;
	uint32 a = pw/8*3;
	uint32 step = 10;
	// sw_line_shrink(&iy[row*pw], 160, &oy[row*ow], 156, temp);
	for(row = 120;row<ph;row++)
	{
		sw_line_shrink(&iy[row*pw], a, &oy[row*ow], a-step, temp);
		sw_line_expend(&iy[row*pw+a], b, &oy[row*ow+a-step], b+2*step, temp);
		sw_line_shrink(&iy[row*pw+a+b], a, &oy[row*ow+a-step+b+2*step], a-step, temp);
		if(row % 10 == 0)
		{
			b += 2*step;
			a -= step;
		}

		if(b == pw || a== 0)
		{
			break;
		}
	}
	printf("row %d\r\n", row);
	// for(row = 0;row<ph/2;row++)
	// {
	// 	sw_line_expend(&iu[row*(pw/2)], pw/2, &ou[row*ow/2], ow/2, temp);
	// 	sw_line_expend(&iv[row*(pw/2)], pw/2, &ov[row*ow/2], ow/2, temp);
	// }
}

void sw_scaler_enlarge1_x(uint8* iyuv, uint16 ph, uint16 pw,
              		      uint8 *oyuv, uint16 oh, uint16 ow)
{
	
	uint16 row;
	
	
	
	uint8 *iy, *iu, *iv;
	uint8 *oy, *ou, *ov;
	
	
	
	uint8 temp[512];
	
	iy = iyuv;
	iu = &iyuv[ph*pw];
	iv = &iyuv[ph*pw+ph*pw/4];
	oy = oyuv;
	ou = &oyuv[oh*ow];
	ov = &oyuv[oh*ow + oh*ow/4];

	for(row = 0;row<ph;row++)
	{
		sw_line_expend(&iy[row*pw], pw, &oy[row*ow], ow, temp);
	}

	for(row = 0;row<ph/2;row++)
	{
		sw_line_expend(&iu[row*(pw/2)], pw/2, &ou[row*ow/2], ow/2, temp);
		sw_line_expend(&iv[row*(pw/2)], pw/2, &ov[row*ow/2], ow/2, temp);
	}
}


inline void data_recfg(int s_i,int s_j,int d_i,int d_j,int s_w,char *sbuf,int cfg_type){
	int *sbuf_32;
	
	sbuf_32 = (int *)sbuf;
	if(cfg_type == 1)					//2*2
	{
		//s_w = s_w*4;
		sbuf_32[s_i+4*s_w*s_j] = sbuf_32[d_i+4*s_w*d_j];
		sbuf_32[(s_i+4*s_w*s_j+s_w)] = sbuf_32[d_i+4*s_w*d_j+s_w];
		sbuf_32[(s_i+4*s_w*s_j+2*s_w)] = sbuf_32[d_i+4*s_w*d_j+2*s_w];
		sbuf_32[(s_i+4*s_w*s_j+3*s_w)] = sbuf_32[d_i+4*s_w*d_j+3*s_w];	
	}
	else if(cfg_type == 2)				//2*1
	{			
		sbuf_32[s_i+4*s_w*s_j] = sbuf_32[d_i+4*s_w*d_j];
		sbuf_32[(s_i+4*s_w*s_j+s_w)] = sbuf_32[d_i+4*s_w*d_j+s_w];
	}
	else if(cfg_type == 3)				//1*1
	{
		sbuf[s_i+s_w*s_j] = sbuf[d_i+s_w*d_j];
	}

}



void reduce_and_reflash(int s_w,int s_h,char *sbuf,int mode_type,int buf_type){
	int cache_w,cache_h;
	int reduce_w,reduce_h;
	int x_size;
	int h_log = 0;
	int i ,j;
	int mode = mode_type;
	int shift_x = 0,shift_y = 0;
	int log = 0;
	cache_w = s_w / 3;				//ƽ������
	x_size = (cache_w+1)/2;			//���ȵ�һ��
	cache_h = 3*x_size - 2;

	reduce_w = s_w / 2 - 1;					 //mid_w
	reduce_h = (s_h - cache_h)/2 - 1;        //start_top

	h_log = x_size;


	while(mode)
	{
		switch(mode){
			case 3:
				shift_x = x_size*-1;
				shift_y = -1 *(2*x_size );
			break;

			case 4:
				shift_x = x_size*1;
				shift_y = -1 *(2*x_size );
			break;

			case 2:
				shift_x = x_size*1;
				shift_y = (2*(x_size - 1)) /*+ x_size*/-3;
			break;	

			case 1:
				shift_x = x_size*-1;
				shift_y = (2*(x_size - 1)) /*+ x_size*/-3;
			break;	

			case 6:
				shift_x = 2*x_size*-1;
				shift_y = 0;
			break;

			case 5:
				shift_x = 2*x_size;
				shift_y = 0;
			break;

			case 0:
				
			return;
			
		}

		

		mode --;
		//printf("shift_x:%d shift_y:%d s_h:%d\n",shift_x,shift_y,s_h);

		log = 1;
		for(j = 0;j < h_log+1;j++){
			//printf("\r\n\r\n\r\n\r\n");
			//printf("log:%d  %d\n",log,reduce_h + shift_y + j);
			for(i = 0;i < log;i++)
			{
				if(reduce_h + shift_y + j >= 0)
				{
					data_recfg((reduce_w  + shift_x + i -j),(j+reduce_h+shift_y),(reduce_w + i-j),(j+reduce_h),s_w,sbuf,buf_type);
				}
				
			}
			log = log+2;
		}


		for(j = 0;j < h_log-1;j++){
			//printf("\r\n\r\n\r\n\r\n");
			for(i = 0;i < h_log*2+1;i++)
			{
				if((reduce_h + shift_y + j + h_log +1>= 0) &&(reduce_h + shift_y + j + h_log +1< s_h))
				{
					data_recfg((reduce_w - h_log + shift_x + i),(j+reduce_h+shift_y + h_log+1),(reduce_w - h_log + i),(j+reduce_h + h_log+1),s_w,sbuf,buf_type);
				}
			}
		}

		
		log = h_log*2+1;	
		for(j = 0;j < h_log+1;j++){
			//printf("\r\n\r\n\r\n\r\n");
			//printf("reduce_h:%d shift_y:%d s_h:%d\n",reduce_h,shift_y,s_h);
			for(i = 0;i < log;i++)
			{	
				if(reduce_h + shift_y + j + 2*h_log< s_h)
				{
					data_recfg((reduce_w - h_log  + shift_x + i +j),(j+reduce_h+shift_y + 2*h_log),(reduce_w - h_log + i+j),(j+reduce_h+2*h_log),s_w,sbuf,buf_type);
				}
			}
			log = log-2;
		}
	
		
	}
}









uint16 *ve_htable;
void hexagon_ve(uint8* in, uint8* tem, uint32 w, uint32 h)
{
	
	static uint8_t flag = 0;
	get_img_retc(in, h, w, 120-60, 160-60, tem, 160, 140);
	// get_img_retc_blkcpy(in, h, w, 0-80+120, 20+70, tem, 160, 140);
	// csi_dcache_invalid_range(in, w*h+w*h/2);
	if(flag == 0){
		flag = 1;
		ve_htable = os_malloc(2*h);
		hexagon_table_init(in, 160, 140, 3.1415*2/3+0.04, 80, ve_htable);
	}
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80, 20, 240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80, 20+140, 240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120, 20-70, 240, 320);
	// // put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120, 20+70, 240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120, 20+140+70, 240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120+120, 20, 240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120+120, 20+140, 240, 320);		

	
	put_img_hexagon2(tem, 160, 140, ve_htable, 80,  in, 0-80, 20, 240, 320);
	put_img_hexagon2(tem, 160, 140, ve_htable, 80,  in, 0-80, 20+140, 240, 320);
	put_img_hexagon2(tem, 160, 140, ve_htable, 80,  in, 0-80+120, 20-70, 240, 320);
	// put_img_hexagon2(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120, 20+70, 240, 320);
	put_img_hexagon2(tem, 160, 140, ve_htable, 80,  in, 0-80+120, 20+140+70, 240, 320);
	put_img_hexagon2(tem, 160, 140, ve_htable, 80,  in, 0-80+120+120, 20, 240, 320);
	put_img_hexagon2(tem, 160, 140, ve_htable, 80,  in, 0-80+120+120, 20+140, 240, 320);	

	csi_dcache_clean_range((uint32_t*)in, w*h+w*h/2);
}

void hexagon_ve1(uint8* in, uint8* tem, uint32 w, uint32 h)
{
	
	static uint8_t flag = 0;
	// get_img_retc(in, h, w, 100, 100, tem, 160*2, 140*2);
	// get_img_retc_blkcpy(in, h, w, 0-80+120, 20+70, tem, 160, 140);
	if(flag == 0){
		flag = 1;
		ve_htable = os_malloc(h*2);
		hexagon_table_init(in, 160, 140, 3.1415*2/3+0.04, 80, ve_htable);
	}
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80, 	       20, 		  240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80,         20+140,    240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120,     20-70,     240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120, 	   20+140+70, 240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120+120, 20,        240, 320);
	// put_img_hexagon1(tem, 160, 140, 3.1415*2/3+0.04, 80,  in, 0-80+120+120, 20+140,    240, 320);		

	mov_img_hexagon(in, h, w, 60, 100, in, h, w, 0-80, 	       20, 		  ve_htable, 160, 140);
	mov_img_hexagon(in, h, w, 60, 100, in, h, w, 0-80,         20+140,    ve_htable, 160, 140);
	mov_img_hexagon(in, h, w, 60, 100, in, h, w, 0-80+120,     20-70,     ve_htable, 160, 140);
	mov_img_hexagon(in, h, w, 60, 100, in, h, w, 0-80+120, 	   20+140+70, ve_htable, 160, 140);
	mov_img_hexagon(in, h, w, 60, 100, in, h, w, 0-80+120+120, 20,        ve_htable, 160, 140);
	mov_img_hexagon(in, h, w, 60, 100, in, h, w, 0-80+120+120, 20+140,    ve_htable, 160, 140);

	
//	mov_img_hexagon(in, h, w, 100, 100, in, 240, 320, 0-160, 40, ve_htable, 160*2, 140*2);
//	mov_img_hexagon(in, h, w, 100, 100, in, 240, 320, 0-160, 40+280, ve_htable, 160*2, 140*2);
//	mov_img_hexagon(in, h, w, 100, 100, in, 240, 320, 0-160+240, 40-140, ve_htable, 160*2, 140*2);
//	mov_img_hexagon(in, h, w, 100, 100, in, 240, 320, 0-160+240, 40+140+280, ve_htable, 160*2, 140*2);
//	mov_img_hexagon(in, h, w, 100, 100, in, 240, 320, 0-160+240+240, 40, ve_htable, 160*2, 140*2);
//	mov_img_hexagon(in, h, w, 100, 100, in, 240, 320, 0-160+240+240, 40+280, ve_htable, 160*2, 140*2);
	csi_dcache_clean_range((uint32_t*)in, w*h+w*h/2);
	csi_dcache_invalid_range((uint32_t*)in, w*h+w*h/2);
}

void hexagon_ve2(uint8* in, uint8* tem, uint32 w, uint32 h)
{
	reduce_and_reflash(80,240/2,(char *)in,	6,2); 	   //640*480	   ====> 160 * 120																		
	// reduce_and_reflash(40,120/2,in+320*240,		    6,2);
	// reduce_and_reflash(40,120/2,in+320*240+320*240/4,  6,2);
	memset(in+320*240, 128, 320*240/4);
	memset(in+320*240+320*240/4, 128, 320*240/4);
	// csi_dcache_clean_range(in, w*h+w*h/2);
	// csi_dcache_invalid_range(in, w*h+w*h/2);
}


void block_9(uint8* in, uint8* tem, uint32 w, uint32 h)
{
    // // get_img_retc(in, h, w, 120-60, 160-60, tem, 80, 108);
	// get_img_retc(in, h, w, 0, 0, tem, 80, 108);
	// csi_dcache_invalid_range(in, w*h+w*h/2);
	// // put_img_retc(tem, 80, 108, in, 0, 0, 240, 320);
    // put_img_retc(tem, 80, 108, in, 0, 108, 240, 320);
    // put_img_retc(tem, 80, 108, in, 0, 108+108, 240, 320);


    // put_img_retc(tem, 80, 108, in, 80, 0, 240, 320);
    // put_img_retc(tem, 80, 108, in, 80, 108, 240, 320);
    // put_img_retc(tem, 80, 108, in, 80, 108+108, 240, 320);
    
    // put_img_retc(tem, 80, 108, in, 160, 0, 240, 320);
    // put_img_retc(tem, 80, 108, in, 160, 108, 240, 320);
    // put_img_retc(tem, 80, 108, in, 160, 108+108, 240, 320);
	// csi_dcache_clean_range(in, w*h+w*h/2);

	uint32 w1 = 0,w2 = 0,w3 = 0;
	if( w == 320) {
		w1 = 100;
		w2 = 120;
		w3 = 100;
	} else if(w ==640){
		w1 = 200;
		w2 = 240;
		w3 = 200;
	}
	mov_img_retc(in, h, w, h/3, w2, 
				 in, h, w, h/3, 0, 
				 h/3, w1);
	mov_img_retc(in, h, w, h/3, w2, 
				 in, h, w, h/3, w1+w2, 
				 h/3, w3);
	mov_img_retc(in, h, w, h/3, w, 
				 in, h, w, 0,  0, 
				 h/3, w);
	mov_img_retc(in, h, w, h/3, w, 
				 in, h, w, 2*h/3, 0, 
				 h/3, w );

	csi_dcache_clean_range((uint32_t*)in, w*h+w*h/2);
	csi_dcache_invalid_range((uint32_t*)in, w*h+w*h/2);
}

void block_4(uint8* in, uint8* tem, uint32 w, uint32 h)
{
    // // get_img_retc(in, h, w, 120-60, 160-60, tem, 120, 160);
	// get_img_retc(in, h, w, 0, 0, tem, 120, 160);
	// csi_dcache_invalid_range(in, w*h+w*h/2);
	// // put_img_retc(tem, 120, 160, in, 0, 0, 240, 320);
    // put_img_retc(tem, 120, 160, in, 120, 160, 240, 320);
    // put_img_retc(tem, 120, 160, in, 0, 160, 240, 320);
	// put_img_retc(tem, 120, 160, in, 120, 0, 240, 320);
	// csi_dcache_clean_range(in, w*h+w*h/2);
	mov_img_retc(in, h, w, 0,   0, 
				 in, h, w, 0,   w/2, h/2, w/2);
	mov_img_retc(in, h, w, 0,   0, 
				 in, h, w, h/2, 0, h/2, w);
	// csi_dcache_clean_range(in, w*h+w*h/2);
	// csi_dcache_invalid_range(in, w*h+w*h/2);
}

void block_2_yinv(uint8* in, uint8* tem, uint32 w, uint32 h)
{
    // get_img_retc(in, h, w, 0, 0, tem, 120, 320);
	// csi_dcache_invalid_range(in, w*h+w*h/2);
	// //put_img_retc(tem, 120, 320, in, 0, 0, 240, 320);
    // put_img_retc_iy(tem, 120, 320, in, 120, 0, 240, 320);
	// csi_dcache_clean_range(in, w*h+w*h/2);

	mov_img_retc_iy(in, h, w, h/2, 0, 
			        in, h, w, 0, 0, 
				    h/2, w);
	csi_dcache_clean_range((uint32_t*)in, w*h+w*h/2);
	csi_dcache_invalid_range((uint32_t*)in, w*h+w*h/2);
}

void block_2_xinv(uint8* in, uint8* tem, uint32 w, uint32 h)
{
    // get_img_retc_blkcpy(in, h, w, 0, 0, tem, 240, 160);
	// get_img_retc(in, h, w, 0, 0, tem, 240, 160);
	
	// csi_dcache_invalid_range(tem, 160*h+160*h/2);
	// put_img_retc(tem, 240, 160, in, 0, 0, 240, 320);
    mov_img_retc_ix(in, h, w, 0, 0, in, h, w, 0, w/2, h, w/2);
	
	csi_dcache_clean_range((uint32_t*)in, w*h+w*h/2);
	csi_dcache_invalid_range((uint32_t*)in, w*h+w*h/2);
}

// void uv_offset(uint8* in, uint8* tmp, uint32 w, uint32 h, int32 uoff, int32 voff)
// {
// 	uint16 s_row, s_col, e_row, e_col;
// 	uint16 row, col;
// 	uint16 i,j;
// 	uint32 uv_len = h*w/4;
// 	uint32 w_len;
// 	uint16 img_row,img_col;
// 	uint8  *iy,*iu,*iv;
// 	iy = in;
// 	iu = &iy[h*w];
// 	iv = &iu[h*w/4];
// 	uint8* line_tmp = os_malloc(128);
// 	if(line_tmp == NULL) {
// 		os_printf("ve_malloc fail\r\n");
// 	}

// 	if(uoff != 0) {
// 		while(uv_len > 0)
// 		{
// 			w_len = uv_len > 128 ? 128 : uv_len;
// 			hw_memcpy(line_tmp, iu, w_len);
// 			for(j = 0;j<w_len;j++) {
// 				if(line_tmp[j] +uoff > 255) {
// 					line_tmp[j] = 255;
// 				} else if(line_tmp[j] +uoff <0) {
// 					line_tmp[j] = 0;
// 				} else {
// 					line_tmp[j] += uoff;
// 				}
// 			}
// 			hw_memcpy(iu, line_tmp, w_len);
// 			uv_len -= w_len;
// 			iu += w_len;
// 		}
// 		csi_dcache_clean_range(iu, w*h/4);
// 		csi_dcache_invalid_range(iu, w*h/4);
// 	} 
// 	uv_len = h*w/4;

// 	if(voff != 0) {
// 		while(uv_len > 0)
// 		{
// 			w_len = uv_len > 128 ? 128 : uv_len;
// 			hw_memcpy(line_tmp, iv, w_len);
// 			for(j = 0;j<w_len;j++) {
// 				if(line_tmp[j] +voff > 255) {
// 					line_tmp[j] = 255;
// 				} else if(line_tmp[j] +voff <0) {
// 					line_tmp[j] = 0;
// 				} else {
// 					line_tmp[j] += voff;
// 				}
// 			}
// 			hw_memcpy(iv, line_tmp, w_len);
// 			uv_len -= w_len;
// 			iv += w_len;
// 		}
// 		csi_dcache_clean_range(iv, w*h/4);
// 		csi_dcache_invalid_range(iv, w*h/4);
// 	} 
// 	os_free(line_tmp);
// }

void uv_offset(uint8* in, uint8* tmp, uint32 w, uint32 h, int32 uoff, int32 voff)
{


	int32 val;
	uint32 j;
	uint32 uv_len = h*w/4;


	uint8  *iy,*iu,*iv;
	iy = in;
	iu = &iy[h*w];
	iv = &iu[h*w/4];


	if(uoff != 0) {
		j = 0;
		while(uv_len > 0)
		{
			val = iu[j] + uoff;
			if(val > 255) {
				iu[j] = 255;
			} else if(val <0) {
				iu[j] = 0;
			} else {
				iu[j] = val;
			}
			uv_len--;
			j++;
		}
	}


	uv_len = h*w/4;
	if(voff != 0) {
		j = 0;
		while(uv_len > 0)
		{
			val = iv[j] + voff;
			if(val > 255) {
				iv[j] = 255;
			} else if(val <0) {
				iv[j] = 0;
			} else {
				iv[j] = val;
			}
			uv_len--;
			j++;
		}

	} 
	csi_dcache_clean_range((uint32_t*)iu, w*h/2);
	csi_dcache_invalid_range((uint32_t*)iu, w*h/2);
	
}

void ve_gray_img(uint8* in, uint8* tmp, uint32 w, uint32 h)
{



	uint32 j;
	uint32 uv_len = h*w/4;
	

	volatile uint8  *iy,*iu,*iv;
	iy = in;
	iu = &iy[h*w];
	iv = &iu[h*w/4];


	j = 0;
	while(uv_len > 0)
	{
		iu[j] = 128;
		uv_len--;
		j++;
	}

	uv_len = h*w/4;
	j = 0;
	while(uv_len > 0)
	{
		iv[j] = 128;
		uv_len--;
		j++;
	}

	csi_dcache_clean_range((uint32_t*)iu, w*h/2);
	csi_dcache_invalid_range((uint32_t*)iu, w*h/2);
}

void vf_switch(uint32 sw)
{
	g_vf_cblk.desp.open = sw;
}

void vf_set_addr(uint32 src, uint32 dst)
{
	g_vf_cblk.p_src = src;
	g_vf_cblk.p_dst = dst;
}

