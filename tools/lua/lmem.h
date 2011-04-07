/*
** $Id$
** Interface to Memory Manager
** See Copyright Notice in lua.h
*/

#ifndef lmem_h
#define lmem_h


#ifndef NULL
#define NULL 0
#endif


/* memory error messages */
#define codeEM   "code size overflow"
#define constantEM   "constant table overflow"
#define refEM   "reference table overflow"
#define tableEM  "table overflow"
#define memEM "not enough memory"

void *luaM_realloc (void *oldblock, unsigned long size);
int luaM_growaux (void **block, unsigned long nelems, int size,
                       char *errormsg, unsigned long limit);

#define luaM_free(b)	free((b))
#define luaM_malloc(t)	malloc((t))
#define luaM_new(t)          ((t *)malloc(sizeof(t)))
#define luaM_newvector(n,t)  ((t *)malloc((n)*sizeof(t)))
#define luaM_growvector(old,n,t,e,l) \
          (luaM_growaux((void**)old,n,sizeof(t),e,l))
#define luaM_reallocvector(v,n,t) ((t *)realloc(v,(n)*sizeof(t)))


#ifdef DEBUG
extern unsigned long numblocks;
extern unsigned long totalmem;
#endif


#endif

