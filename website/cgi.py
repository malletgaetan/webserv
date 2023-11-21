import sys
import os

body_a = """
<!DOCTYPE html>
<html>
<head>
  <title>CGI</title>
</head>
<body>
  <h1>This page was dynamically generated using python CGI!!</h1>
  <p>The python was execute from directory \""""

body_b = """\"</p>
</body>
</html>
"""

body = f"{body_a}{os.getcwd()}{body_b}"

sys.stdout.write("HTTP/1.1 200 OK\r\n")
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write(f"Content-Length: {len(body)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
