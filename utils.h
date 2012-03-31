/* 
 * File:   utils.h
 * Author: Adrian Jamróz
 */

#ifndef __UTILS_H
#define	__UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TRUE		1
#define FALSE		0
#define	LINEMAX		1024
#define	GRANULARITY	1024
#define N(x)		(sizeof(x) / sizeof(x[0]))
#define XASSERT(x) \
	do { \
		if (!x) \
			printf("assertion failed on %s:%d", __FILE__, __LINE__); \
	} while (0)

#define FOREACH_ARRAY(array, i, data) \
    for (i = 0, data = array_count(array) ? array_get(array, 0) : NULL; i < array_count(array); i++, data = array_get(array, i))

struct array 
{
	void		**a_data;
	int		a_count;
};

typedef struct array array_t;

array_t	*array_init(int);
array_t	*array_init_with(const void *[], size_t);
array_t	*array_init_split_string(char *, const char *);
void	array_free(array_t *, int, int);
void	array_append(array_t *, void *);
void	array_prepend(array_t *, void *);
void	array_remove(array_t *, int);
void	*array_get(array_t *, int);
void	array_set(array_t *, int, void *);
void	array_resize(array_t *, int);
int		array_count(array_t *);


#define FOREACH_HASH(hash, i, key, value) \
	for(i = 0, key = hash_count(hash) ? hash->h_data[0].h_key : NULL, \
		value = hash_count(hash) ? hash->h_data[0].h_data : NULL; \
		i < hash_count(hash); i++, key = hash->h_data[i].h_key, \
		value = hash->h_data[i].h_data)


struct hash_kv
{
	const char	*h_key;
	void		*h_data;
};

typedef struct hash_kv hash_kv_t;

struct hash
{
	hash_kv_t	*h_data;
	int		h_count;
};

typedef struct hash hash_t;

hash_t	*hash_init();
void	hash_free(hash_t *, int, int);
void	*hash_get(hash_t *, const char *);
char	*hash_get_string(hash_t *, const char *);
int	hash_get_int(hash_t *, const char *);
void	hash_set(hash_t *, const char *, void *, int);
int	hash_unset(hash_t *, const char *, int);
int	hash_key_exists(hash_t *, const char *);
int	hash_count(hash_t *);

#define	NEW(type, destructor)			(type *)managed_new(sizeof(type), (void (*)(void*))destructor)
#define	DELETE(data) 				managed_delete(data)
#define	MANAGED 				void (*m_destructor)(void *)

void	*managed_new(size_t, void (*)(void*));
void	managed_delete(void *);

void	*xmalloc(size_t);
void	*xcmalloc(size_t);
void	*xrealloc(void *, size_t);
char	*xstrdup(const char *);
char	*xsubstrdup(const char *, int, int);
int	*xintdup(int);
char	*xstrcat(char *, const char *);
char	*xfgetln(FILE *);
int	xprintf(const char *, ...);
int	xread(FILE *, char **, int, void (*)(int, int));
int	xread_chunked(FILE *, char **, void (*)(int, int));
array_t *regexp_match(const char *, const char *, int);
char	*strip_html(char *);

/*
 * ISO C90 aka ANSI C nie lubi makr z varargsami...
 */
void	FAIL(const char *, ...);

#endif	/* __UTILS_H */

