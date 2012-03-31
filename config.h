/*
 * File:   config.h
 * Author: Adrian Jamróz
 */
    
#ifndef __CONFIG_H
#define	__CONFIG_H

#include "utils.h"
#include "storage.h"

hash_t	*config_get_feeds(storage_handle_t *);
char	*config_get(storage_handle_t *, const char *);
hash_t	*config_get_all(storage_handle_t *);
void	config_set(storage_handle_t *, const char *, const char *);

#endif	/* __CONFIG_H */

