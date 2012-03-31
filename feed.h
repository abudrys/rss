/* 
 * File:   feed.h
 * Author: Adrian Jamróz
 */

#ifndef __FEED_H
#define	__FEED_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"
#include "storage.h"

#define	QUERY_HAS_SOURCE	0x1
#define QUERY_HAS_LIMIT		0x2
#define	QUERY_HAS_FROM_TIME	0x4
#define	QUERY_HAS_TO_TIME	0x8
#define	QUERY_ALL		0x10

#define UNIX_MINUTE		60
#define UNIX_HOUR		(UNIX_MINUTE * 60)
#define UNIX_DAY		(UNIX_HOUR * 24)

struct feed
{
	MANAGED;
	char	*f_name;
	char	*f_url;
	char	*f_description;
	time_t	f_last_update;
};

typedef struct feed feed_t;

struct feed_entry
{
	MANAGED;
	char	*fe_feed;
	char	*fe_title;
	char	*fe_url;
	char	*fe_description;
	time_t	fe_pubdate;  
};

typedef struct feed_entry feed_entry_t;

struct feed_query
{
	MANAGED;
	int	fq_mask;
	char	*fq_feed;
	time_t	fq_from_time;
	time_t	fq_to_time;
	int	fq_limit;
	int	fq_count;
};

typedef struct feed_query feed_query_t;

feed_t	*feed_create(char *);
void	feed_free(feed_t *);
void	feed_entry_free(feed_entry_t *);
void	feed_save(storage_handle_t *, feed_t *);
void	feed_remove(storage_handle_t *, feed_t *);
void	feed_download(storage_handle_t *, feed_t *);
void	feed_flush(storage_handle_t *, time_t);
array_t	*feed_get_entries(storage_handle_t *, feed_query_t *);

#endif	/* __FEED_H */

