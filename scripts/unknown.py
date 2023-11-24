import http.client
import sys

def delete_file(host):
    try:
        connection = http.client.HTTPConnection(host)
        connection.request('UNKNOWN', '/')
        response = connection.getresponse()

        if response.status == 200:
            print(f"File deleted successfully. Status Code: {response.status}")
        else:
            print(f"Failed to delete file. Status Code: {response.status}")

        connection.close()
    except http.client.HTTPException as e:
        print(f"Request failed: {e}")

# Replace 'host' and 'path' with your server details and file path
server_host = sys.argv[1]

delete_file(server_host)
