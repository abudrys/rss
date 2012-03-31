/* 
 * File:   storage.h
 * Author: Adrian Jamróz
 */

#ifndef __STORAGE_H
#define	__STORAGE_H

#include <time.h>
#include <sqlite3.h>
#include "utils.h"

#define STORAGE_CREATE_CONFIG_SQL						\
	"CREATE TABLE config ("							\
	"	name VARCHAR(255) NOT NULL PRIMARY KEY,"			\
	"	value LONGVARCHAR"						\
	");"

#define STORAGE_CREATE_FEEDS_SQL						\
	"CREATE TABLE feeds ("							\
	"	name VARCHAR(255) NOT NULL PRIMARY KEY,"			\
	"	url VARCHAR(255) NOT NULL,"					\
	"	description LONGVARCHAR,"					\
	"	updated TIMESTAMP"						\
	");"
			
#define STORAGE_CREATE_POSTS_SQL						\
	"CREATE TABLE posts ("							\
	"	id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"			\
	"	feed VARCHAR(255) NOT NULL,"					\
	"	pubdate TIMESTAMP,"						\
	"	title VARCHAR(255) NOT NULL UNIQUE,"				\
	"	url VARCHAR(255) NOT NULL,"					\
	"	description LONGVARCHAR"					\
	");"

#define	QUERY_HAS_SOURCE	0x1
#define	QUERY_HAS_LIMIT		0x2
#define QUERY_HAS_FROM_TIME	0x4
#define QUERY_HAS_TO_TIME	0x8

static const struct {
	const char *name;
	const char *value;
} storage_variables[] = {
	{ "use_pager", "on" },
	{ "use_colors", "on" }
};

struct storage_handle
{
	sqlite3		*sh_db;
};

typedef struct storage_handle storage_handle_t;
typedef sqlite3_stmt storage_stmt_t;

storage_handle_t *storage_get();
storage_stmt_t	*storage_query(storage_handle_t *, const char *);
int		storage_step(storage_stmt_t *, hash_t **);
void		storage_finalize(storage_stmt_t *);
void		storage_initialize(storage_handle_t *);
void		storage_close(storage_handle_t *);

#endif	/* __STORAGE_H */

