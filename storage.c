/* 
 * File:   utils.c
 * Author: Adrian Jamróz
 */

#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sqlite3.h>
#include "utils.h"
#include "storage.h"
#include "globals.h"

storage_handle_t *storage_get()
{
        /*
	 * Globalny wskaźnik do otwartej bazy danych sqlite3.
	*/
	static storage_handle_t *handle = NULL;

	if (!handle) {
		handle = xmalloc(sizeof(storage_handle_t));
		sqlite3_open(db_location, &(handle->sh_db));
	}
	
	return handle;
}

storage_stmt_t *storage_query(storage_handle_t *handle, const char *query)
{
	sqlite3_stmt *stmt;

#ifdef SQLITE_DEBUG
	fprintf(stderr, "SQLITE3 DEBUG: storage_query(\"%s\");\n", query);
#endif
	
	if (sqlite3_prepare(handle->sh_db, query, strlen(query), &stmt, NULL) != SQLITE_OK) {
		FAIL("błąd sqlite3: %s\n", sqlite3_errmsg(handle->sh_db));
		sqlite3_close(handle->sh_db);
		exit(EXIT_FAILURE);
	}
	
	return (storage_stmt_t *)stmt;
}

int storage_step(storage_stmt_t *stmt, hash_t **row)
{
	int i, ret, ncols = sqlite3_column_count(stmt);
	void *data;
	ret = sqlite3_step(stmt);
	
	if (ret != SQLITE_ROW) {
		errno = EINVAL;
		return ret;
	}
	
	if (!row || ret == SQLITE_DONE)
		return ret;
	
	*row = hash_init();
	
	for (i = 0; i < ncols; i++) {	
		switch (sqlite3_column_type(stmt, i)) {
			case SQLITE_INTEGER:
				data = xintdup(sqlite3_column_int(stmt, i));
				break;
				
			case SQLITE_NULL:
				data = NULL;
				break;

			case SQLITE_BLOB:
			case SQLITE_TEXT:
				data = xstrdup((char *)sqlite3_column_text(stmt, i));
				break;
				
			default:
				FAIL("błąd sqlite3: nierozpoznany typ danych. Prawdopodobnie jest to błąd wewnętrzny programu.\n");
				exit(EXIT_FAILURE);
		}
		
		hash_set(*row, xstrdup(sqlite3_column_name(stmt, i)), data, TRUE);
	}
		
	return ret;
}

void storage_finalize(storage_stmt_t *stmt)
{
	sqlite3_finalize(stmt);
}

void storage_initialize(storage_handle_t *handle)
{
	int i;
	char *sql;
	char *error;
	
	if (sqlite3_exec(handle->sh_db, STORAGE_CREATE_CONFIG_SQL, NULL, NULL, &error) != SQLITE_OK)
	    goto fail;
	
	if (sqlite3_exec(handle->sh_db, STORAGE_CREATE_FEEDS_SQL, NULL, NULL, &error) != SQLITE_OK)
	    goto fail;

    	if (sqlite3_exec(handle->sh_db, STORAGE_CREATE_POSTS_SQL, NULL, NULL, &error) != SQLITE_OK)
	    goto fail;

	for (i = 0; i < N(storage_variables); i++) {
		sql = sqlite3_mprintf(
		    "INSERT INTO config VALUES (%Q, %Q)", 
		    storage_variables[i].name, 
		    storage_variables[i].value);
		
		if (sqlite3_exec(handle->sh_db, sql, NULL, NULL, &error))
		    goto fail;
	}

	xprintf("Zainicjalizowano nową bazę danych.\n");
	return;
	
fail:
	FAIL("błąd sqlite3: nie udało się zainicjować tabeli: %s", sqlite3_errmsg(handle->sh_db));
	exit(EXIT_FAILURE);
}

void storage_close(storage_handle_t *handle)
{
	sqlite3_close(handle->sh_db);
	free(handle);
}
