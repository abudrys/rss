/*
 * File:   http.c
 * Author: Adrian Jamróz
 */    

#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <regex.h>
#include <errno.h>
#include <string.h>
#include "utils.h"
#include "http.h"
#include "feed.h"

int	http_parse_uri(http_request_t *, const char *);
int	http_do_request(http_request_t *, http_response_t *);
void	http_read_callback(int, int);

int http_parse_uri(http_request_t *req, const char *uri)
{
	int status;
	array_t *regexp = regexp_match("http://([^/]+)/(.*)", uri, REG_EXTENDED|REG_ICASE);

	if (!regexp) {
		if (errno != EINVAL)
			FAIL("nie udało się dopasować wyrażenia regularnego na \"%s\"\n", uri);
	
		return 0;
	}

	addrinfo_t hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM
	};
	
	if (req->hr_hostname) free(req->hr_hostname);
	if (req->hr_path) free(req->hr_path);
	if (req->hr_addrinfo) freeaddrinfo(req->hr_addrinfo);
    
	req->hr_hostname = xstrdup(array_get(regexp, 1));
	req->hr_port = htons(80);
	req->hr_path = xstrdup(array_get(regexp, 2));
	
	if ((status = getaddrinfo(req->hr_hostname, "http", &hints, &req->hr_addrinfo))) {
		FAIL("Nie udało się rozwiązać domeny %s: %s\n", req->hr_hostname, gai_strerror(status));
		return 0;
	}
	
	array_free(regexp, TRUE, FALSE);
	return -1;
}

http_request_t *http_new_request(const char *url, hash_t *headers)
{
	http_request_t *req = xcmalloc(sizeof(http_request_t));
	req->hr_headers = headers;
	
	if (!http_parse_uri(req, url))
		return NULL;
	
	return req;
}

void http_free_request(http_request_t *req)
{
	if (req->hr_response) {
		http_response_t *resp = req->hr_response;
		hash_free(resp->hs_headers, TRUE, FALSE);
		free(resp->hs_body);
		free(resp);
	}

	if (req->hr_addrinfo) 
		freeaddrinfo(req->hr_addrinfo);	
	
	hash_free(req->hr_headers, TRUE, FALSE);
	free(req->hr_hostname);
	free(req->hr_path);
	free(req);
}

http_response_t *http_send_request(http_request_t *req)
{
	http_response_t *resp = xcmalloc(sizeof(http_response_t));
	resp->hs_headers = hash_init();
	req->hr_response = resp;
	http_do_request(req, resp);
	return resp;
}

int http_do_request(http_request_t *req, http_response_t *resp)
{
	int i, sock, status;
	FILE *fsock;
	addrinfo_t *ptr;
	const char *key;
	void *value;
    
	for (ptr = req->hr_addrinfo; ptr; ptr = ptr->ai_next) {
		
		struct sockaddr_in *sin = (struct sockaddr_in *)ptr->ai_addr;
		xprintf("Łączę się z %s:%d... ", inet_ntoa(sin->sin_addr), ntohs(sin->sin_port));
		
		if (!(sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))) {
			FAIL("nie udało się otworzyć gniazda: %s\n", strerror(errno));
			return FALSE;
		}
		
		if (connect(sock, ptr->ai_addr, ptr->ai_addrlen)) {
			FAIL("nie udało się połączyć z serwerem: %s\n", strerror(errno));
			continue;
		};

		fsock = fdopen(sock, "a+");
		fprintf(fsock, "GET /%s HTTP/1.1\r\n", req->hr_path);

		FOREACH_HASH(req->hr_headers, i, key, value) {
			fprintf(fsock, "%s: %s\r\n", key, (char *)value);
		}

		fprintf(fsock, "Host: %s\r\n\r\n", req->hr_hostname);

		if (fscanf(fsock, "HTTP/1.%*1d %3d %*[^\r\n]\r\n", &status) < 1) {
			FAIL("http: niepoprawna odpowiedź serwera.\n");
			errno = EINVAL;
			free(resp);
			return FALSE;
		}

		while (1) {
			char *line = xfgetln(fsock);
			char name[LINEMAX], value[LINEMAX];

			if (line[0] == '\n') {
				free(line);
				break;
			}

			if (sscanf(line, "%[^:\n]: %[^\r\n]\r\n", name, value) < 2) {
				free(line);
				break;
			}

			hash_set(resp->hs_headers, xstrdup(name), (void *)xstrdup(value), TRUE);	
			free(line);
		}
		
		if (status == 301 || status == 302) {
			if (!http_parse_uri(req, hash_get_string(resp->hs_headers, "Location")))
				return FALSE;
			
			xprintf("przekierowanie: %s\n", hash_get_string(resp->hs_headers, "Location"));
			
			fclose(fsock);
			shutdown(sock, SHUT_RDWR);
			return http_do_request(req, resp);
		}
		
		xprintf("pobieram:     ");

		if (!strcmp(hash_get_string(resp->hs_headers, "Transfer-Encoding"), "chunked")) {
			/* 
			 * Odpowiedź jest zakodowana jako "chunki", zgodnie ze specyfikacją
			 * HTTP/1.1 - RFC2616.
			 */
			xread_chunked(fsock, &(resp->hs_body), http_read_callback);
		} else {
			/*
			 * Odpowiedź odczytujemy tak jak w HTTP/1.0, oczekując końca strumienia
			 * (zamknięcia połączenia przez drugą stronę).
			 */
			int nbytes = hash_key_exists(resp->hs_headers, "Content-Length")
				? atoi(hash_get_string(resp->hs_headers, "Content-Length"))
				: -1;
			
			xread(fsock, &(resp->hs_body), (nbytes ? nbytes : -1), http_read_callback);
		}
		
		xprintf("\n");
		fclose(fsock);
		return status;
	}
	
	FAIL("nie udało się znaleźć odpowiedniego serwera.\n");
	return FALSE;
}

void http_read_callback(int nbytes, int count)
{
	if (count != -1) {
		int x = (double)nbytes / (double)count * 100.0;
		printf("\b\b\b\b%3d%%", x);
		fflush(stdout);
	} else {
		printf("\b\b\b\b%2dKB", nbytes / 1024);
		fflush(stdout);
	}
}
