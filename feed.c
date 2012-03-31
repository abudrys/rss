/*
 * File:   feed.c
 * Author: Adrian Jamróz
 */
     
#define _XOPEN_SOURCE
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "utils.h"
#include "feed.h"
#include "config.h"
#include "http.h"
#include "storage.h"

feed_t *feed_create(char *url)
{
	feed_t *feed = NEW(feed_t, feed_free);
	feed->f_url = url;
	return feed;
}

void feed_free(feed_t *feed)
{
	if (feed->f_name) free(feed->f_name);
	if (feed->f_description) free(feed->f_description);
	if (feed->f_url) free(feed->f_url);
}

feed_entry_t *feed_entry_create()
{
	feed_entry_t *entry = NEW(feed_entry_t, feed_entry_free);
	return entry;
}

void feed_entry_free(feed_entry_t *entry)
{
	if (entry->fe_feed) free(entry->fe_feed);
	if (entry->fe_title) free(entry->fe_title);
	if (entry->fe_url) free(entry->fe_url);
	if (entry->fe_description) free(entry->fe_description);
}

int feed_entry_persist(storage_handle_t *handle, feed_entry_t *entry)
{
	int ret;
	char *sql = sqlite3_mprintf(
		"INSERT OR REPLACE INTO posts VALUES (NULL, %Q, %u, %Q, %Q, %Q);",
		entry->fe_feed,
		entry->fe_pubdate,
		entry->fe_title,
		entry->fe_url,
		entry->fe_description
	);
	
	storage_stmt_t *stmt = storage_query(handle, sql);
	
	if ((ret = storage_step(stmt, NULL)) != SQLITE_DONE) {
		FAIL("błąd sqlite3: %s\n", sqlite3_errmsg(handle->sh_db));
		return FALSE;
	}
	
	storage_finalize(stmt);
	sqlite3_free(sql);
	return TRUE;
}

int feed_process(feed_entry_t *entry, xmlNode *node)
{
	char *pubdate_str;
	struct tm pubdate;
	xmlNode *ptr;
	
	if (strcmp((char *)node->name, "item")) {
		errno = EINVAL;
		return FALSE;
	}
	
	for (ptr = node->children; ptr; ptr = ptr->next) {
		if (!strcmp((char *)ptr->name, "pubDate")) {
			pubdate_str = (char *)xmlNodeGetContent(ptr);
		    	entry->fe_pubdate = (!strptime(pubdate_str, "%a, %d %b %Y %T", &pubdate))
		    	    ? time(NULL)
		    	    : mktime(&pubdate);
		
			free(pubdate_str);
		}
		
		if (!strcmp((char *)ptr->name, "title")) entry->fe_title = (char *)xmlNodeGetContent(ptr);
		if (!strcmp((char *)ptr->name, "link")) entry->fe_url = (char *)xmlNodeGetContent(ptr);
		if (!strcmp((char *)ptr->name, "description")) { 
			char *description = strip_html((char *)xmlNodeGetContent(ptr));
			entry->fe_description = description;
		}
	}
	
	return TRUE;
}

void feed_save(storage_handle_t *handle, feed_t *feed)
{
	char *sql = sqlite3_mprintf(
		"INSERT OR REPLACE INTO feeds VALUES (%Q, %Q, %Q, %q)",
		feed->f_name,
		feed->f_url,
		feed->f_description,
		feed->f_last_update
	);
	
	storage_stmt_t *stmt = storage_query(handle, sql);
	storage_step(stmt, NULL);
	storage_finalize(stmt);
	sqlite3_free(sql);
}

void feed_remove(storage_handle_t *handle, feed_t *feed)
{
	char *sql = sqlite3_mprintf("DELETE FROM posts WHERE feed = %Q", feed->f_name);
	storage_stmt_t *stmt = storage_query(handle, sql);
	storage_step(stmt, NULL);
	storage_finalize(stmt);
	sqlite3_free(sql);

	sql = sqlite3_mprintf("DELETE FROM feeds WHERE name = %Q", feed->f_name);
	stmt = storage_query(handle, sql);
	storage_step(stmt, NULL);
	storage_finalize(stmt);
	sqlite3_free(sql);
}

void feed_download(storage_handle_t *handle, feed_t *feed)
{
	int done = 0;
	hash_t *headers = hash_init();
	http_request_t *request;
	http_response_t *response;
	xmlDoc *document;
	xmlNode *root, *ptr, *ptr0;
	
	hash_set(headers, xstrdup("User-Agent"), xstrdup("rss/0.0.2"), FALSE);

	if (!(request = http_new_request(feed->f_url, headers))) {
		if (errno == EINVAL) {
		    FAIL("Podany URL: \"%s\" jest nieprawidłowy.\n", feed->f_url);
		    return;
		}
		
		return;
	}
	
	response = http_send_request(request);
	document = xmlReadDoc((uint8_t *)response->hs_body, NULL, "UTF-8", XML_PARSE_NOCDATA);
	root = xmlDocGetRootElement(document);
	
	if (!document) {
		FAIL("libxml2: dokument pod podanym adresem jest nieprawidłowy.\n");
		http_free_request(request);
		return;
	}
	
	for (ptr = root->children; ptr; ptr = ptr->next) {
		if (!strcmp((char *)ptr->name, "channel")) {
			for (ptr0 = ptr->children; ptr0; ptr0 = ptr0->next) {
				feed_entry_t *entry = feed_entry_create();
				entry->fe_feed = xstrdup(feed->f_name);

				if (!feed_process(entry, ptr0)) {
					DELETE(entry);
					continue;
				}
				
				if (!feed_entry_persist(handle, entry)) {
					DELETE(entry);
					continue;
				}	
				
				DELETE(entry);
				done++;	
			}
		}
	}
		
	xmlFreeDoc(document);	
	http_free_request(request);
	printf("Zapisano %d nowych wiadomości ze źródła %s.\n", done, feed->f_name);
}

void feed_flush(storage_handle_t *handle, time_t amount)
{
	char *sql = sqlite3_mprintf("DELETE FROM posts WHERE pubdate <= %u\n", amount);
	storage_stmt_t *stmt = storage_query(handle, sql);
	storage_step(stmt, NULL);
	storage_finalize(stmt);
	sqlite3_free(sql);
}

array_t *feed_get_entries(storage_handle_t *handle, feed_query_t *query)
{
	array_t *ret;
	hash_t *row;
	feed_entry_t *entry;
	int any_where = FALSE;
	char *sql = sqlite3_mprintf("SELECT * FROM posts");
	char *saved_sql;
	storage_stmt_t *stmt;
    
	if (query->fq_mask & QUERY_HAS_SOURCE || 
	    query->fq_mask & QUERY_HAS_FROM_TIME || 
	    query->fq_mask & QUERY_HAS_TO_TIME) {
		saved_sql = sql;
		sql = sqlite3_mprintf("%s WHERE ", sql);
		sqlite3_free(saved_sql);
	}
	
	if (query->fq_mask & QUERY_HAS_SOURCE) {
		saved_sql = sql;
		sql = sqlite3_mprintf("%s feed = %Q", sql, query->fq_feed);
		any_where = TRUE;
		sqlite3_free(saved_sql);
	}
	
	if (query->fq_mask & QUERY_HAS_FROM_TIME) {
		saved_sql = sql;
		sql = sqlite3_mprintf("%s %s pubdate >= %u", sql, any_where++ ? " AND " : "", sql, query->fq_from_time);
		sqlite3_free(saved_sql);
	}
	
	if (query->fq_mask & QUERY_HAS_TO_TIME) {
		saved_sql = sql;
		sql = sqlite3_mprintf("%s %s pubdate <= %u", sql, any_where++ ? " AND " : "", query->fq_to_time);
		sqlite3_free(saved_sql);
	}
    
	if (query->fq_mask & QUERY_HAS_LIMIT) {
		saved_sql = sql;
		sql = sqlite3_mprintf("%s LIMIT %d", sql, query->fq_limit);
		sqlite3_free(saved_sql);
	}

	if (!query->fq_mask) {
		saved_sql = sql;
		sql = sqlite3_mprintf("%s WHERE pubdate >= %u", sql, time(NULL) - UNIX_DAY);
		sqlite3_free(saved_sql);
	}

	ret = array_init(0);
	stmt = storage_query(handle, sql);
	
	while (storage_step(stmt, &row) == SQLITE_ROW) {
		time_t *pubdate = (time_t *)hash_get(row, "pubdate");
		entry = feed_entry_create();
		entry->fe_pubdate = *pubdate;
		entry->fe_feed = hash_get_string(row, "feed");
		entry->fe_title = hash_get_string(row, "title");
		entry->fe_url = hash_get_string(row, "url");
		entry->fe_description = hash_get_string(row, "description");
		free(hash_get(row, "id"));
		free(hash_get(row, "pubdate"));
		hash_free(row, FALSE, FALSE);
		array_append(ret, (void *)entry);
	}

	storage_finalize(stmt);
	sqlite3_free(sql);
	return ret;
}
