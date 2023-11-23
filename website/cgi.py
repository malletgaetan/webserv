import sys
import os

def get_method(request):
    # Split the request into lines
    lines = request.split('\r\n')
    # Parse the request line
    request_line = lines[0].split(' ')
    method = request_line[0]
    return method

body_a = """
<!DOCTYPE html>
<html>
<head>
  <title>CGI</title>
</head>
<body>
  <h1>This page was dynamically generated using python CGI!!</h1>
  <p>The python was execute from directory \""""

body_b = """</p>
</body>
</html>
"""

body = f"{body_a}{os.getcwd()}\"\n\t<p>got method {get_method(sys.stdin.read())}</p>\n{body_b}"

sys.stdout.write("HTTP/1.1 200 OK\r\n")
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write(f"Content-Length: {len(body)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)

