/****************************************************************************/
/* stdarg.h v4.4.1                                                          */
/*                                                                          */
/* Copyright (c) 1996-2012 Texas Instruments Incorporated                   */
/* http://www.ti.com/                                                       */
/*                                                                          */
/*  Redistribution and  use in source  and binary forms, with  or without   */
/*  modification,  are permitted provided  that the  following conditions   */
/*  are met:                                                                */
/*                                                                          */
/*     Redistributions  of source  code must  retain the  above copyright   */
/*     notice, this list of conditions and the following disclaimer.        */
/*                                                                          */
/*     Redistributions in binary form  must reproduce the above copyright   */
/*     notice, this  list of conditions  and the following  disclaimer in   */
/*     the  documentation  and/or   other  materials  provided  with  the   */
/*     distribution.                                                        */
/*                                                                          */
/*     Neither the  name of Texas Instruments Incorporated  nor the names   */
/*     of its  contributors may  be used to  endorse or  promote products   */
/*     derived  from   this  software  without   specific  prior  written   */
/*     permission.                                                          */
/*                                                                          */
/*  THIS SOFTWARE  IS PROVIDED BY THE COPYRIGHT  HOLDERS AND CONTRIBUTORS   */
/*  "AS IS"  AND ANY  EXPRESS OR IMPLIED  WARRANTIES, INCLUDING,  BUT NOT   */
/*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   */
/*  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT   */
/*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,   */
/*  SPECIAL,  EXEMPLARY,  OR CONSEQUENTIAL  DAMAGES  (INCLUDING, BUT  NOT   */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   */
/*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY   */
/*  THEORY OF  LIABILITY, WHETHER IN CONTRACT, STRICT  LIABILITY, OR TORT   */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   */
/*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                          */
/****************************************************************************/

#ifndef _STDARG
#define _STDARG

#ifdef __cplusplus
namespace std {
#endif /* __cplusplus */

typedef char *va_list;

#ifdef __cplusplus
} /* namespace std */
#endif

#if !defined(__TMS320C55X_PLUS_BYTE__)
#define va_start(ap, parmN)                     \
    _Pragma("diag_suppress 238")                \
 	(ap = ((char *)__va_parmadr(parmN)) + 	\
	((__va_argref(parmN) ? sizeof(&parmN) : sizeof(parmN))-sizeof(int))) \
    _Pragma("diag_default 238")
#else
#define va_start(ap, parmN)                                \
    _Pragma("diag_suppress 238")                           \
	(ap = ((char *)__va_parmadr(parmN)) +              \
	((__va_argref(parmN) ? sizeof(&parmN) :            \
	    (sizeof(parmN) < 2 ? 2 : sizeof(parmN)))))     \
    _Pragma("diag_default 238")
#endif

#define va_end(ap) ((void)0)

/****************************************************************************/
/* VA_ARG - Return pointer to the next argument                             */
/*                                                                          */
/* Conventions:                                                             */
/*   1) An argument of size greater then one word is aligned on an even     */
/*      word boundary.                                                      */
/*   2) Argument pointer points to the last word of the previous argument   */
/*      and is updated to point to the last word of the current argument.   */
/*                                                                          */
/* When an argument is passed by-reference (indicated by __va_argref being  */
/* true) the actual argument passed is a pointer and will be treated much   */
/* the same as a "void *" argument.                                         */
/*                                                                          */
/* The first expression of the outer comma expression adds in any necessary */
/* stack alignment per convention 1) above.                                 */
/*                                                                          */
/* The second expression of the outer comma expression increments the       */
/* argument pointer to point to the last word of the argument and produces  */
/* the address to use for the argument.                                     */
/****************************************************************************/

#if !defined(__TMS320C55X_PLUS_BYTE__)
#define va_arg(ap, type)  (						     \
    _Pragma("diag_suppress 238")					     \
    __va_argref(type) ?							     \
      ((ap) += (sizeof(void *) > 1 && ( ! ((long)(ap) & 1)))) :		     \
      ((ap) += (sizeof(type  ) > 1 && ( ! ((long)(ap) & 1)))),		     \
    __va_argref(type) ?							     \
      ((ap) += sizeof(void *), (** (type **) ((ap) - (sizeof(void *)-1)))) : \
      ((ap) += sizeof(type  ), (*  (type *)  ((ap) - (sizeof(type  )-1))))   \
    _Pragma("diag_default 238")						     \
    )
#else

/*****************************************************************************/
/* objects of size "char" are widened to "int" before being passed to a	     */
/* variadic function.  ints are aligned to 2 bytes, longs and long longs     */
/* to 4. aggregates are handled as pointers, which in huge model are 4	     */
/* bytes.								     */
/*****************************************************************************/
/* Note: ap is left pointing to the *beginning* of where the next arg might  */
/* occur								     */
/*****************************************************************************/
#define va_arg(ap, type)  (						      \
    _Pragma("diag_suppress 238")					      \
    __va_argref(type) ?							      \
      ((ap) = (char *)(((long)ap + 3) & ~3)) :                                \
      ((ap) = (char *)                                                        \
	      ((sizeof(type) >= sizeof(long)) ? (((long)ap + 3) & ~3) :       \
	       (                                ((long)ap + 1) & ~1))),       \
    __va_argref(type) ?							      \
      ((ap) += 4, (** (type **) ((ap) - 4))) :                                \
      ((ap) += ((sizeof(type) < 2) ? 2 : sizeof(type)),                       \
       *(type*)((ap) - ((sizeof(type) < 2 ? 2 : sizeof(type)))))              \
    _Pragma("diag_default 238")						      \
    )

#endif

#endif /* _STDARG */

#if defined(__cplusplus) && !defined(_CPP_STYLE_HEADER)
using std::va_list;
#endif /* _CPP_STYLE_HEADER */

