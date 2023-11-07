#include "http.hpp"

namespace HTTP {
	std::map<int, std::string> errors;

	void  init_errors(void)
	{
		std::string badRequest = "<html>"
			"<head>"
				"<title>400 Bad Request</title>"
			"</head>"
			"<body>"
				"<h1>Bad Request</h1>"
			"</body>"
			"</html>";
		errors[HTTP_BAD_REQUEST] = badRequest;

		std::string notFound = "<html>"
			"<head>"
				"<title>404 Not Found</title>"
			"</head>"
			"<body>"
				"<h1>Not Found</h1>"
			"</body>"
			"</html>";
		errors[HTTP_NOT_FOUND] = notFound;

		std::string internalServerError = "<html>"
			"<head>"
				"<title>500 Internal Server Error</title>"
			"</head>"
			"<body>"
				"<h1>Internal Server Error</h1>"
			"</body>"
			"</html>";
		errors[HTTP_INTERNAL_SERVER_ERROR] = internalServerError;

		std::string requestTimeoutError = "<html>"
			"<head>"
				"<title>408 Request Timeout</title>"
			"</head>"
			"<body>"
				"<h1>Request Timeout</h1>"
			"</body>"
			"</html>";
		errors[HTTP_REQUEST_TIMEOUT_ERROR] = requestTimeoutError;

		std::string methodNotAllowed = "<html>"
			"<head>"
				"<title>405 Method Not Allowed</title>"
			"</head>"
			"<body>"
				"<h1>Method Not Allowed</h1>"
			"</body>"
			"</html>";
		errors[HTTP_METHOD_NOT_ALLOWED] = methodNotAllowed;
	}

	const std::string *default_error(int http_status)
	{
		return &(errors[http_status]);
	}
}
