/* 
 * File:   main.c
 * Author: Adrian Jamróz
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <langinfo.h>
#include <errno.h>
#include "globals.h"
#include "storage.h"
#include "utils.h"
#include "cli.h"

void usage();
void version();

void usage()
{
        fprintf(stderr, "Użycie: rss [-h] [-v] [-d <plik>]\n");
	fprintf(stderr, "\t-h - wyświetla ten komunikat.\n");
	fprintf(stderr, "\t-v - wyświetla informacje o wersji.\n");
	fprintf(stderr, "\t-d <ścieżka do pliku> - używa alternatywnego pliku z bazą danych (domyślnie ~/.rss.db).\n");
	exit(EXIT_SUCCESS);
}

void version()
{
	xprintf("rss 0.0.1.\n");
	xprintf("(c) 2008 Adrian Jamróz.\n");
}

int main(int argc, char** argv) 
{
	int ch;
	struct passwd *pwd;
	struct stat dbstat;
	db_location = NULL;
	
        while ((ch = getopt(argc, argv, "hd:v")) != -1) {
		switch (ch) { 
			case 'h':
				usage();
				break;
            
			case 'd':
				db_location = xstrdup(optarg);
				break;
			
			case 'v':
				version();
				exit(EXIT_SUCCESS);
		}
	}
	
	if (!db_location) {
		pwd = getpwuid(getuid());
		asprintf(&db_location, "%s/%s", pwd->pw_dir, RSS_DB_FILENAME);
	}
	
	if (stat(db_location, &dbstat) < 0) {
		if (errno == ENOENT) 
			storage_initialize(storage_get());
	}
	
	version();
	cli_mainloop();
	return EXIT_SUCCESS;
}
