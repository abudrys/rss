/* 
 * File:   cli.c
 * Author: Adrian Jamróz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <readline/readline.h>
#include "utils.h"
#include "config.h"
#include "feed.h"
#include "globals.h"
#include "http.h"
#include "help.h"

void	cli_sigpipe(int);
void	do_help(array_t *);
void	do_add_source(array_t *);
void	do_remove_source(array_t *);
void	do_list_sources(array_t *);
void	do_update(array_t *);
void	do_view(array_t *);
void	do_flush(array_t *);
void	do_set(array_t *);
void	do_about(array_t *);
void	do_exit(array_t *);
time_t	parse_time_diff(const char *);

struct callback_table {
	char	*ct_command;
	void	(*ct_function)(array_t *);
};

struct callback_table cli_callbacks[] = {
	{ "help", do_help },
	{ "add", do_add_source },
	{ "remove", do_remove_source },
	{ "list", do_list_sources },
	{ "update", do_update },
	{ "view", do_view },
	{ "flush", do_flush },
	{ "set", do_set },
	{ "about", do_about },
	{ "exit", do_exit },
};

void cli_mainloop()
{
	signal(SIGPIPE, cli_sigpipe);
	
	while (1) {
		int i;
		int found = 0;
		char *line = readline("rss> ");
		array_t *args = array_init_split_string(line, " ");

		if (array_count(args) < 1)
			continue;
		
		for(i = 0; i < N(cli_callbacks); i++) {
			if (!strcmp(cli_callbacks[i].ct_command, array_get(args, 0))) {
				cli_callbacks[i].ct_function(args);
				array_free(args, TRUE, FALSE);
				found = 1;
				break;
			}
			
			found = 0;
		}
		
		if (!found)
			xprintf("Nieznane polecenie. Aby zobaczyć pomoc, wpisz 'help'.\n");

		free(line);
	}
}

int cli_ask(const char *question)
{
	char ch;
	xprintf("%s ", question);
	ch = getchar();
	
	switch (ch) {
		case 't': case 'y': case 'T': case '\n':
			return TRUE;
		case 'n': case 'N':
			return FALSE;
		default:
			return cli_ask(question);
	}
}

void cli_sigpipe(int signo)
{
	/*
	 * Zapewne użytkownik nacisnął 'q' w pagerze.
	 * można zignorować.
	 */
	return;
}

void do_help(array_t *args)
{
	int i;
	
	if (array_count(args) == 1) {
		xprintf("Dostępne polecenia:\n");
		for (i = 0; i < N(cli_callbacks); i++)
			xprintf("\t%s\n", cli_callbacks[i].ct_command);
			
		xprintf("Aby uzyskać informację na temat wybranego polecenia,\n");
		xprintf("wpisz: 'help <nazwa_polecenia>'.\n");
		return;
	}
	
	if (array_count(args) == 2) {
		for (i = 0; i < N(usage_texts); i++) {
			if (strcmp(usage_texts[i].ud_cmd, array_get(args, 1)))
				continue;
			
			xprintf("Użycie: %s\n\n", usage_texts[i].ud_syntax);
			xprintf("%s", usage_texts[i].ud_usage);
			return;
		}
		
		xprintf("help: nie ma takiego polecenia. aby uzyskać pomoc, wpisz 'help'.\n");
		return;
	}
	
	xprintf("help: aby uzyskać pomoc, wpisz po prostu 'help'.\n");
}

void do_add_source(array_t *args)
{
	feed_t *feed;
	
	if (array_count(args) != 3) {
		FAIL("add: należy podać dwa argumenty! (aby uzyskać pomoc "
		     "na temat tego polecenia, wpisz: help add)\n");
		return;
	}
	
	feed = feed_create(xstrdup(array_get(args, 2)));
	feed->f_name = xstrdup(array_get(args, 1));
	feed_save(storage_get(), feed);
	
	if (cli_ask("Czy pobrać nowe wpisy z nowo dodanego źródła? [T/n]"))
		feed_download(storage_get(), feed);
}

void do_remove_source(array_t *args)
{
	const char *name;
	hash_t *feeds = config_get_feeds(storage_get());

	if (array_count(args) != 2) {
		FAIL("remove: należy podać jeden argument! (aby uzyskać pomoc "
	    	     "na temat tego polecenia, wpisz: help remove)\n");
		return;
	}
	
	name = array_get(args, 1);

	if (hash_key_exists(feeds, name))
		feed_remove(storage_get(), hash_get(feeds, name));

	hash_free(feeds, TRUE, TRUE);
	xprintf("Źródło %s zostało usunięte.\n", name);
}

void do_list_sources(array_t *args)
{
	int i;
	const char *key;
	void *value;
	hash_t *feeds = config_get_feeds(storage_get());
	
	xprintf("Nazwa źródła (URL):\n");
	
	FOREACH_HASH(feeds, i, key, value) {
		feed_t *feed = (feed_t *)value;
		xprintf("%s (%s)\t%s\n", key, feed->f_url, feed->f_description);
		
	}

	hash_free(feeds, TRUE, TRUE);
}

void do_update(array_t *args)
{
	hash_t *feeds = config_get_feeds(storage_get());
	const char *name;
	void *value;
	feed_t *feed;
	int i;

	if (array_count(args) == 2) {
		if (!hash_key_exists(feeds, array_get(args, 1))) {
			xprintf("Nie ma źródła o nazwie %s.\n", array_get(args, 1));
			return;
		}
		
		feed = hash_get(feeds, array_get(args, 1));
		xprintf("Aktualizacja źródła: %s\n", feed->f_name);
		feed_download(storage_get(), feed);
	}
	
	if (array_count(args) == 1) {
		FOREACH_HASH(feeds, i, name, value) {
			feed = (feed_t *)value;
			xprintf("Aktualizacja źródła: %s\n", (char *)name);
			feed_download(storage_get(), feed);
		} 
	}

	hash_free(feeds, TRUE, TRUE);
}

void do_view(array_t *args)
{
	int use_colors, i, limit = 0;
	char *use_pager = NULL;
	void *data;
	feed_query_t fq = { 0 };
	FILE *f = NULL;
	feed_entry_t *fe;
	array_t *entries = NULL;
	hash_t *feeds = config_get_feeds(storage_get());
	
	use_colors = !strcmp(config_get(storage_get(), "use_colors"), "on");
	
	FOREACH_ARRAY(args, i, data) {
		char *cmd = (char *)data;
		
		if (!strcmp(cmd, "feed")) {
			fq.fq_mask = QUERY_HAS_SOURCE;
			fq.fq_feed = xstrdup(array_get(args, ++i));
		}
		
		if (!strcmp(cmd, "newer")) {
			fq.fq_mask |= QUERY_HAS_FROM_TIME;
			fq.fq_from_time = parse_time_diff(array_get(args, ++i));
		}
		
		if (!strcmp(cmd, "older")) {
			fq.fq_mask |= QUERY_HAS_TO_TIME;
			fq.fq_to_time = parse_time_diff(array_get(args, ++i));
		}

		if (!strcmp(cmd, "limit")) {
			if (!(limit = strtoul(array_get(args, ++i), NULL, 10))) {
				xprintf("limit: proszę podać wartość numeryczną.\n");
				goto cleanup;
			}
			
			fq.fq_mask |= QUERY_HAS_LIMIT;
			fq.fq_limit = limit;
		}

		if (!strcmp(cmd, "all"))
			fq.fq_mask = QUERY_ALL;
	}
	
	entries = feed_get_entries(storage_get(), &fq);
	
	if (array_count(entries) == 0) {
		printf("Nie znaleziono pasujących wiadomości.\n");
		goto cleanup;
	}
	
	use_pager = config_get(storage_get(), "use_pager");
	f = (!strcmp(use_pager, "on"))
	    ? popen(PAGER, "w")
	    : stdout;
	    
	FOREACH_ARRAY(entries, i, data) {
		char date[256];
		struct tm *tmp;
		fe = (feed_entry_t *)data;

		tmp = localtime(&(fe->fe_pubdate));
		strftime(date, sizeof(date), "%A, %d %B %Y, %H:%M:%S", tmp);
	
		if (use_colors) {
			fprintf(f, "\033[1mŹródło: %s\033[0m\n", fe->fe_feed);
			fprintf(f, "\033[1mData: %s \033[0m\n", date);
			fprintf(f, "\033[1mURL: %s\033[0m\n", fe->fe_url);
			fprintf(f, "\033[1;31m%s\033[0m\n", fe->fe_title);
		} else {
			fprintf(f, "Źródło: %s\n", fe->fe_feed);
			fprintf(f, "Data: %s\n", date);
			fprintf(f, "URL: %s\n", fe->fe_url);
			fprintf(f, "Tytuł: %s\n", fe->fe_title);
		}
		
		fprintf(f, "%s\n", fe->fe_description);
		fprintf(f, "\n");
	}
	
cleanup:
	if (f && f != stdout)
	        fclose(f);
	
	free(use_pager);
	array_free(entries, TRUE, TRUE);
	hash_free(feeds, TRUE, TRUE);
}

void do_flush(array_t *args)
{
	if (array_count(args) != 2) {
		printf("Niepoprawna składnia. Aby uzyskać pomoc na temat tego polecenia, wpisz 'help flush'.\n");
		return;
	}
	
	feed_flush(storage_get(), parse_time_diff(array_get(args, 1)));
	xprintf("Usunięto wiadomości starsze niż %s.\n", array_get(args, 1));
	return;
}

void do_set(array_t *args)
{
	if (array_count(args) == 1) {
		/* Wyświetl zawartość wszystkich zmiennych */
		int i;
		const char *key;
		void *value;
		hash_t *config = config_get_all(storage_get());
		
		FOREACH_HASH(config, i, key, value) {
			xprintf("%s: %s\n", key, (char *)value);
		}

		hash_free(config, TRUE, FALSE);
	}
	
	if (array_count(args) == 2) {
		char *varname = array_get(args, 1);
		if (varname[0] == '-') {
			char *old;
			/* Ustaw NULLa. */
			varname++;
			old = config_get(storage_get(), varname);
			config_set(storage_get(), varname, NULL);
			xprintf("%s: %s -> <null>\n", varname, old);
		} else {
			/* Wyświetl zawartość zmiennej */
			xprintf("%s: %s\n", varname, config_get(storage_get(), varname));
		}
		
		return;
	}
	
	if (array_count(args) == 3) {
		/* Wyświetl i ustaw zawarość zmiennej */
		char *varname = array_get(args, 1);
		char *old = config_get(storage_get(), varname);
		config_set(storage_get(), varname, array_get(args, 2));
		xprintf("%s: %s -> %s\n", varname, *old != '\0' ? old : "<brak>" , (char *)array_get(args, 2));
		return;
	}
	
}

void do_about(array_t *args)
{
	xprintf("rss - lekki, konsolowy czytnik kanałów RSS.\n");
	xprintf("(c) 2008 Adrian Jamróz\n");
	xprintf("Wersja 0.0.1, październik 2008.\n");
}

void do_exit(array_t *args)
{
	storage_close(storage_get());
	exit(EXIT_SUCCESS);
}

time_t parse_time_diff(const char *str)
{
	char *modifier;
	time_t ret;
	
	if (!(ret = strtoul(str, &modifier, 10)))
		return (time_t)0;
	    
	if (*modifier == 'd')
		return (time_t)(time(NULL) - (ret * UNIX_DAY));
		
	if (*modifier == 'h')
		return (time_t)(time(NULL) - (ret * UNIX_HOUR));
	
	if (*modifier == 'm')
		return (time_t)(time(NULL) - (ret * UNIX_MINUTE));
	
	return (time_t)(time(NULL) - (ret * UNIX_DAY));
}
