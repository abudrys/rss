/* 
 * File:   http.h
 * Author: Adrian Jamróz
 */

#ifndef __HTTP_H
#define	__HTTP_H

#include <netinet/in.h>
#include <stdint.h>
#include "utils.h"

struct http_uri;
struct http_request;
struct http_response;

typedef struct addrinfo addrinfo_t;
typedef struct http_uri http_uri_t;
typedef struct http_request http_request_t;
typedef struct http_response http_response_t;

struct http_uri
{
        char			*hu_scheme;
        char			*hu_hostname;
        char			*hu_path;
        u_int16_t		hu_port;
};

struct http_request
{
	char			*hr_hostname;
	char			*hr_path;
	u_int16_t		hr_port;
	hash_t			*hr_headers;
	addrinfo_t		*hr_addrinfo;
	http_response_t	*hr_response;
};

struct http_response
{
        int			hs_status;
	char			*hs_body;
	hash_t			*hs_headers;
        http_request_t	*hs_request;
};

http_request_t *http_new_request(const char *, hash_t *);
http_response_t *http_send_request(http_request_t *);
void http_free_request(http_request_t *);

#endif	/* __HTTP_H */
