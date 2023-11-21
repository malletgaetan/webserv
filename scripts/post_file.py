import urllib.request
import sys

if len(sys.argv)

url = sys.argv[1]  # Replace with your endpoint
file_path = sys.argv[2]  # Replace with the path to your file

with open(file_path, 'rb') as file:
    file_content = file.read()

    # Create a POST request with the file content as data
    request = urllib.request.Request(url, data=file_content)
    request.add_header('Content-Type', 'application/octet-stream')

    # Send the request
    response = urllib.request.urlopen(request)

    if response.getcode() == 200:
        print("File uploaded successfully.")
    else:
        print("Upload failed with status code:", response.getcode())
