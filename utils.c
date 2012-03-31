/* 
 * File:   utils.c
 * Author: Adrian Jamróz
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdarg.h>
#include "globals.h"
#include "utils.h"

array_t *array_init(int size)
{
	array_t *array = xcmalloc(sizeof(array_t));
	array->a_data = xmalloc(sizeof(void *) * size);
	array->a_count = size;
	return array;
}
 
array_t *array_init_with(const void *data[], size_t count)
{
	array_t *array = array_init(count);
	memcpy(array->a_data, data, count * sizeof(void *));
	return array;
}

array_t *array_init_split_string(char *string, const char *delims)
{
	array_t *ret = array_init(0);
	char *token;
	
	while ((token = strsep(&string, delims))) {
		if (strlen(token))
			array_append(ret, (void *)xstrdup(token));
	}
	
	free(string);
	return ret;
}

void array_free(array_t *array, int deep, int managed)
{
	if (!array)
		return;

	if (deep) {
		int i;
		void *data;
		FOREACH_ARRAY(array, i, data) {
			if (managed) DELETE(data);
			else free(data);
		}
	}
	
	free(array->a_data);
	free(array);
}

void array_append(array_t *array, void *data)
{
	array->a_count++;
	array->a_data = xrealloc(array->a_data, array->a_count * sizeof(void *));
	array->a_data[array->a_count-1] = data;
}

void array_prepend(array_t *array, void *data)
{
	array->a_count++;
	array->a_data = xrealloc(array->a_data, array->a_count * sizeof(void *));
	memmove((void *)(array->a_data + 1), array->a_data, array->a_count * sizeof(void *));
}

void *array_get(array_t *array, int index)
{
	if (index >= array->a_count) {
		errno = ENOENT;
		return NULL;
	}
	
	return array->a_data[index];
}

void array_set(array_t *array, int index, void *data)
{
	if (index > array->a_count)
		array_resize(array, index);
	
	array->a_data[index] = data;
}

void array_resize(array_t *array, int size)
{
	if (array->a_count > size) {
		errno = EINVAL;
		return;
	}
	
	array->a_data = xrealloc(array->a_data, size * sizeof(void *));
	array->a_count = size;
}

int array_count(array_t *array)
{
	return array->a_count;
}

hash_t *hash_init()
{
	hash_t *hash = xcmalloc(sizeof(hash_t));
	return hash;
}

void hash_free(hash_t *hash, int deep, int managed)
{
	int i;
	const char *key;
	void *value;

	if (!hash)
		return;

	FOREACH_HASH(hash, i, key, value) {
		free((char *)key);
		if (deep) {
			if (managed) DELETE(value);
		        else free(value);
		}
	}
	
	free(hash->h_data);
	free(hash);
}

void *hash_get(hash_t *hash, const char *skey)
{
	int i;
	const char *key;
	void *value;
	
	FOREACH_HASH(hash, i, key, value) {
		if (!strcmp(key, skey))
			return value;
	}
	
	return NULL;
}

char *hash_get_string(hash_t *hash, const char *skey)
{
	char *ret = (char *)hash_get(hash, skey);
	return ret ? ret : xstrdup("");
}

int hash_get_int(hash_t *hash, const char *skey)
{
	int *ret = (int *)hash_get(hash, skey);
	return ret ? *ret : 0;
}

void hash_set(hash_t *hash, const char *key, void *value, int free_old)
{
	int i;
	hash_kv_t *item = NULL;
	
	for (i = 0; i < hash_count(hash); i++) {
		if (!strcmp(key, hash->h_data[i].h_key))
			item = &(hash->h_data[i]);
	}
	
	if (item && free_old) {
		free((char *)item->h_key);
		free(item->h_data);
	}
	
	if (!item) {
		hash->h_count++;
		hash->h_data = xrealloc(hash->h_data, (hash->h_count + 1) * sizeof(hash_kv_t));
		item = (hash_kv_t *)&(hash->h_data[hash->h_count - 1]);
	}
	
	item->h_key = key;
	item->h_data = value;
}

int hash_key_exists(hash_t *hash, const char *skey)
{
	int i;
	const char *key;
	void *value;
	
	FOREACH_HASH(hash, i, key, value) {
		if (!strcmp(key, skey))
			return TRUE;
	}
	
	return FALSE;
}

int hash_count(hash_t *hash)
{
	if (!hash) {
	    errno = EINVAL;
	    return 0;
	}
	
	return hash->h_count;
}

void *managed_new(size_t nbytes, void (*destructor)(void *))
{
	void *ret = xcmalloc(nbytes);
	struct { MANAGED; } *tmp = ret;
	tmp->m_destructor = destructor;
	return ret;
}

void managed_delete(void *data)
{
	struct { MANAGED; } *tmp = data;	
	tmp->m_destructor(tmp);
	free(tmp);
}

void *xmalloc(size_t nbytes)
{
	void *ptr = malloc(nbytes);
	XASSERT(ptr);
	return ptr;
}

void *xcmalloc(size_t nbytes)
{
	void *ptr = xmalloc(nbytes);
	memset(ptr, 0, nbytes);
	return ptr;
}

void *xrealloc(void *ptr, size_t nbytes)
{
	ptr = realloc(ptr, nbytes);
	XASSERT(ptr);
	return ptr;
}

char *xfgetln(FILE *f)
{
	uint32_t nbytes = LINEMAX;
	char *buf = xcmalloc(nbytes + 1);
	getline(&buf, &nbytes, f);
	return buf;
}

int xprintf(const char *format, ...)
{
	int ret;
	va_list args;
	va_start(args, format);
	ret = vprintf(format, args);
	va_end(args);
	return ret;
}

int xread(FILE *f, char **buf, int nbytes, void (*callback)(int, int))
{
	int ret, todo, done = 0;
	
	while (done < nbytes || nbytes == -1) {
		todo = (nbytes == -1 || nbytes - done >= GRANULARITY) ? GRANULARITY : nbytes-done;
		*buf = xrealloc(*buf, sizeof(char) * (done + todo + 1));
		memset((void *)(*buf + done), 0, todo + 1);
		if ((ret = fread((void *)(*buf + done), 1, todo, f)) < 1) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
		}
				
		done += ret;
		
		if (callback) callback(done, nbytes);
		if (feof(f)) break;
	}
	
	return done;
}

int xread_chunked(FILE *f, char **buf, void (*callback)(int, int))
{
	char *line;
	int ret, done = 0;
	uint32_t todo;

	do {
		line = xfgetln(f);
		
		if (sscanf(line, "%x\r", &todo) < 1) {
			free(line);
			continue;
		}
		
		*buf = xrealloc(*buf, sizeof(char) * (done + todo + 1));
		if ((ret = fread((void *)(*buf + done), 1, todo, f)) < 1) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
		}
			
		done += ret;
		free(line);
		
		if (callback) callback(done, -1);
		if (feof(f)) break;
	} while (0);
	
	return done;	
}

char *xstrdup(const char *str)
{
	char *ret = strdup(str);
	XASSERT(ret);
	return ret;
}

char *xsubstrdup(const char *str, int start, int end)
{
	char *ret = xcmalloc(sizeof(char) * (end - start + 1));
	strncpy(ret, (char *)(str + start), end - start);
	return ret;
}

int *xintdup(int number)
{
	int *ret = xmalloc(sizeof(int));
	*ret = number;
	return ret;
}

char *xstrcat(char *dst, const char *src)
{
	size_t len = strlen(dst) + strlen(src) + 1;
	char *ret = xcmalloc(len);
	strcpy(ret, dst);
	strcat(ret, src);
	return ret;
}

array_t *regexp_match(const char *pattern, const char *str, int cflags)
{
	int i, status;
	regex_t regexp;
	regmatch_t *regmatch;
	array_t *ret = array_init(0);
	
	if (regcomp(&regexp, pattern, cflags)) {
		FAIL("błąd wewnętrzny: cannot compile regular expression ,,%s''\n", pattern);
		exit(EXIT_FAILURE);
	}
	
	regmatch = xcmalloc(sizeof(regmatch_t) * (regexp.re_nsub+1));
	
	if ((status = regexec(&regexp, str, regexp.re_nsub+1, regmatch, 0))) {
		char error[LINEMAX];
		regerror(status, &regexp, error, LINEMAX);
		regfree(&regexp);
		errno = EINVAL;
		return NULL;
	}
	
	for (i = 0; i < regexp.re_nsub+1; i++) {
		array_append(ret, (void *)xsubstrdup(str, regmatch[i].rm_so, regmatch[i].rm_eo));
	}

	free(regmatch);	
	regfree(&regexp);
	return ret;
}

char *strip_html(char *text)
{
	int i;
	char *orig = text;
	char *ret = xcmalloc(strlen(text) + 1);
	int done = 0, in_tag = FALSE, entity = FALSE;
	
	struct { const char *seq; const char ch; } entities[] = {
		{ "lt;", '<' },
		{ "gt;", '>' },
		{ "amp;", '&' }
	};	
	
	do {
		if (*text == '<') {
			in_tag = TRUE;
			continue;
		}
		
		if (*text == '>' && in_tag) {
			in_tag = FALSE;
			continue;
		}
		
		if (*text == '&') {
			entity = FALSE;
			for (i = 0; i < N(entities); i++) {
				if (!strncmp(text+1, entities[i].seq, strlen(entities[i].seq))) {
					ret[done++] = entities[i].ch;
					text = strchr(text, ';');
					entity = TRUE;
					break;
				}
			}
			
			if (entity)
				continue;
		}
		
		if (!in_tag) {
			ret[done++] = *text; 
		}
	} while (*text++);
	
	free(orig);
	return ret;
}

void FAIL(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}
