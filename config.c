/*
 * File:   config.c
 * Author: Adrian Jamróz
 */
    
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h>
#include <sqlite3.h>
#include "storage.h"
#include "utils.h"
#include "feed.h"

hash_t *config_get_feeds(storage_handle_t *handle)
{
	storage_stmt_t *stmt = storage_query(handle, "SELECT * FROM feeds");
	hash_t *row, *ret = hash_init();
	
	while (storage_step(stmt, &row) == SQLITE_ROW) {
		feed_t *feed = feed_create(hash_get(row, "url"));
		feed->f_name = hash_get_string(row, "name");
		feed->f_description = hash_get_string(row, "description");
		feed->f_last_update = hash_get_int(row, "updated");
		hash_set(ret, xstrdup(hash_get(row, "name")), feed, TRUE);
		hash_free(row, FALSE, FALSE);
	}
	
	storage_finalize(stmt);
	return ret;
}

char *config_get(storage_handle_t *handle, const char *name)
{
	char *ret, *sql = sqlite3_mprintf("SELECT * FROM config WHERE name = %Q", name);
	storage_stmt_t *stmt = storage_query(handle, sql);
	hash_t *row = NULL;
	
	storage_step(stmt, &row);
	storage_finalize(stmt);
	sqlite3_free(sql);
	ret = row ? hash_get(row, "value") : xstrdup("");

	if (!ret)
		ret = xstrdup("<brak>");
	
	if (row) {
		free(hash_get(row, "name"));
	        hash_free(row, FALSE, FALSE);
	}
	
	return ret;
}

hash_t *config_get_all(storage_handle_t *handle)
{
	char *sql = "SELECT * FROM config";
	hash_t *row, *ret = hash_init();
	storage_stmt_t *stmt = storage_query(handle, sql);
	
	while (storage_step(stmt, &row) == SQLITE_ROW) {
		hash_set(
			ret, 
			hash_get(row, "name"),
			hash_get(row, "value"),
			TRUE
		);
		
		hash_free(row, FALSE, FALSE);
	}
	
	storage_finalize(stmt);
	return ret;
}

void config_set(storage_handle_t *handle, const char *name, const char *value)
{
	char *sql = sqlite3_mprintf("INSERT OR REPLACE INTO config VALUES (%Q, %Q)", name, value);
	storage_stmt_t *stmt = storage_query(handle, sql);
	storage_step(stmt, NULL);
	storage_finalize(stmt);
	sqlite3_free(sql);
}
