import sys

body = """
<!DOCTYPE html>
<html>
<head>
  <title>Example Page</title>
</head>
<body>
  <h1>Welcome to the Example Page</h1>
  <p>This is an example HTML content.</p>
</body>
</html>
"""

sys.stdout.write("HTTP/1.1 200 OK\r\n")
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write(f"Content-Length: {len(body)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
