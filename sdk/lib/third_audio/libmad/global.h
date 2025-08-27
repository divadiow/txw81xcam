/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: global.h,v 1.11 2004/01/23 09:41:32 rob Exp $
 */

# ifndef LIBMAD_GLOBAL_H
# define LIBMAD_GLOBAL_H

#define FORCE_MONO_CHANNEL    0

#define FPM_DEFAULT
#define NDEBUG
#include "sys_config.h"

#include "typesdef.h"
#define HAVE_ASSERT_H

typedef void     *(*mp3_malloc)(int size);
typedef void (*mp3_free)(void *ptr);
typedef void *(*mp3_calloc)(int nmemb,int size);


extern mp3_malloc g_mp3_malloc;
extern mp3_free   g_mp3_free;
extern mp3_calloc g_mp3_calloc;
void reg_mp3_malloc(mp3_malloc m,mp3_free f,mp3_calloc c);


/* conditional debugging */

# if defined(DEBUG) && defined(NDEBUG)
#  error "cannot define both DEBUG and NDEBUG"
# endif

# if defined(DEBUG)
#  include <stdio.h>
# endif

/* conditional features */

# if defined(OPT_SPEED) && defined(OPT_ACCURACY)
#  error "cannot optimize for both speed and accuracy"
# endif

# if defined(OPT_SPEED) && !defined(OPT_SSO)
#  define OPT_SSO
# endif

# if defined(HAVE_UNISTD_H) && defined(HAVE_WAITPID) &&  \
    defined(HAVE_FCNTL) && defined(HAVE_PIPE) && defined(HAVE_FORK)
#  define USE_ASYNC
# endif

# if !defined(HAVE_ASSERT_H)
#  if defined(NDEBUG)
#   define assert(x)	/* nothing */
#  else
#   define assert(x)	do { if (!(x)) abort(); } while (0)
#  endif
# endif

# endif
