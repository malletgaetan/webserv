#pragma once

#include <map>
#include <string>

#define HTTP_NOT_FOUND 404
#define HTTP_BAD_REQUEST 400
#define HTTP_INTERNAL_SERVER_ERROR 500
#define HTTP_REQUEST_TIMEOUT_ERROR 308
#define HTTP_METHOD_NOT_ALLOWED 405

namespace HTTP {
	enum Method {
		GET,
		HEAD,
		POST,
		DELETE
	};
	void  init_errors(void);
	const std::string *default_error(int http_status);
}