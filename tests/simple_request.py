import http.client

conn = http.client.HTTPConnection('localhost', 8000)
conn.request('BO', "/")
response = conn.getresponse()
print(response.status, response.reason)
