#pragma once

#include <map>
#include <string>


// TODO: macros to enum?

// TODO: missing HTTP_PAYLOAD_TOO_LARGE (body_limit)
// TODO: missing NOT_ACCEPTABLE

#define HTTP_OK 200
#define HTTP_PERMANENT_REDIRECT 308
#define HTTP_BAD_REQUEST 400
#define HTTP_NOT_FOUND 404
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_REQUEST_TIMEOUT_ERROR 408
#define HTTP_INTERNAL_SERVER_ERROR 500
#define HTTP_GATEWAY_TIMEOUT 504

namespace HTTP {
	enum Method {
		GET,
		HEAD,
		POST,
		DELETE
	};
	void  init_maps(void);
	const std::string &default_error(int http_status);
	const std::string &status_definition(int http_status);
	const std::string &mime_type(const std::string &format);
}
