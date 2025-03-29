# Include the libraries for socket and system calls
import socket
import sys
import os
import argparse
import re


# 1MB buffer size
BUFFER_SIZE = 1000000

# Get the IP address and Port number to use for this web proxy server
parser = argparse.ArgumentParser()
parser.add_argument('hostname', help='the IP Address Of Proxy Server')
parser.add_argument('port', help='the port number of the proxy server')
args = parser.parse_args()
proxyHost = args.hostname
proxyPort = int(args.port)

# Create a server socket, bind it to a port and start listening
try:
  # Create a server socket
  # ~~~~ INSERT CODE ~~~~
  #listen_socket = socket.socket(socket.AF_INET, socket. 0); the one from the lecture video 
  serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  # ~~~~ END CODE INSERT ~~~~
  print ('Created socket')
except:
  print ('Failed to create socket')
  sys.exit()

try:
  # Bind the the server socket to a host and port
  # ~~~~ INSERT CODE ~~~~
  #bind(listen_socket, server_addr->ai_addr, server_addr->ai_addrlen) # just like the on from lecture
  serverSocket.bind((proxyHost, proxyPort))
  # ~~~~ END CODE INSERT ~~~~
  print ('Port is bound')
except:
  print('Port is already in use')
  sys.exit()

try:
  # Listen on the server socket
  # ~~~~ INSERT CODE ~~~~
  serverSocket.listen(5) # since on lecture she put QLEN = 5 so lets try 5 here too 
  # ~~~~ END CODE INSERT ~~~~
  print ('Listening to socket')
except:
  print ('Failed to listen')
  sys.exit()

# continuously accept connections
while True:
  print ('Waiting for connection...')
  clientSocket = None

  # Accept connection from client and store in the clientSocket
  try:
    # ~~~~ INSERT CODE ~~~~
    #connection_socket = accept(listen_socket, NULL, NULL); from the lecture 
    clientSocket, clientAddr = serverSocket.accept()
    # ~~~~ END CODE INSERT ~~~~
    print ('Received a connection')
  except:
    print ('Failed to accept connection')
    sys.exit()

  # Get HTTP request from client
  # and store it in the variable: message_bytes
  # ~~~~ INSERT CODE ~~~~
  #recv(connection_socket, &character, sizeof(char), 0); Lecture code 

  message_bytes = clientSocket.recv(BUFFER_SIZE)

  # ~~~~ END CODE INSERT ~~~~
  message = message_bytes.decode('utf-8')
  print ('Received request:')
  print ('< ' + message)

  # Extract the method, URI and version of the HTTP client request 
  requestParts = message.split()
  method = requestParts[0]
  URI = requestParts[1]
  version = requestParts[2]

  print ('Method:\t\t' + method)
  print ('URI:\t\t' + URI)
  print ('Version:\t' + version)
  print ('')

  # Get the requested resource from URI
  # Remove http protocol from the URI
  URI = re.sub('^(/?)http(s?)://', '', URI, count=1)

  # Remove parent directory changes - security
  URI = URI.replace('/..', '')

  # Split hostname from resource name
  resourceParts = URI.split('/', 1)
  hostname = resourceParts[0]
  resource = '/' + resourceParts[1] if len(resourceParts) == 2 else '/'

  #if len(resourceParts) == 2:
    # Resource is absolute URI with hostname and resource
   # resource = resource + resourceParts[1]

  print ('Requested Resource:\t' + resource)

  # Check if resource is in cache
  #try:
  safe_resource = re.sub(r'[^\w\-_./]', '_', resource)
  cacheLocation = './' + hostname + safe_resource
  if cacheLocation.endswith('/'):
      cacheLocation = cacheLocation + 'default'

  print ('Cache location:\t\t' + cacheLocation)

    #fileExists = os.path.isfile(cacheLocation)
  try: 
    # Check wether the file is currently in the cache
    cacheFile = open(cacheLocation, "r")
    cacheData = cacheFile.readlines()

    print ('Cache hit! Loading from cache file: ' + cacheLocation)
    # ProxyServer finds a cache hit
    # Send back response to client 
    # ~~~~ INSERT CODE ~~~~

    clientSocket.sendall(''.join(cacheData).encode())

    # ~~~~ END CODE INSERT ~~~~
    cacheFile.close()
    print ('Sent to the client:')
    print ('> ' + cacheData)
  except:
    # cache miss.  Get resource from origin server
    originServerSocket = None
    # Create a socket to connect to origin server
    # and store in originServerSocket
    # ~~~~ INSERT CODE ~~~~

    originServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    originServerSocket.connect((hostname, 80))
    # ~~~~ END CODE INSERT ~~~~

    print ('Connecting to:\t\t' + hostname + '\n')
    try:
      # Get the IP address for a hostname
      address = socket.gethostbyname(hostname)
      # Connect to the origin server
      # ~~~~ INSERT CODE ~~~~

      originServerRequest = f"GET {resource} HTTP/1.1"
      originServerRequestHeader = f"Host: {hostname}\r\nConnection: close\r\n"
      request = originServerRequest + '\r\n' + originServerRequestHeader + '\r\n\r\n'
      originServerSocket.sendall(request.encode())

      # ~~~~ END CODE INSERT ~~~~
      print ('Connected to origin Server')

      #originServerRequest = ''
      #originServerRequestHeader = ''
      # Create origin server request line and headers to send
      # and store in originServerRequestHeader and originServerRequest
      # originServerRequest is the first line in the request and
      # originServerRequestHeader is the second line in the request
      # ~~~~ INSERT CODE ~~~~

      originServerRequest = f"GET {resource} HTTP/1.1"
      originServerRequestHeader = f"Host: {hostname}\r\nConnection: close"

      # ~~~~ END CODE INSERT ~~~~

      # Construct the request to send to the origin server
      request = originServerRequest + '\r\n' + originServerRequestHeader + '\r\n\r\n'
      originServerSocket.sendall(request.encode())

      # Request the web resource from origin server
      print ('Forwarding request to origin server:')
      for line in request.split('\r\n'):
        print ('> ' + line)

      response_data = originServerSocket.recv(BUFFER_SIZE)
      header_part = response_data.split(b"\r\n\r\n")[0]
      status_code = header_part.split(b" ")[1]

      if status_code in [b'301', b'302']:
        print(f"{status_code.decode()} Redirect detected")
        location_header = [line for line in header_part.split(b"\r\n") if b"Location:" in line]
        if location_header:
          location_url = location_header[0].decode().split(": ",1)[1]
          print(f"Redirecting to: {location_url}")

          new_parts = re.sub('^http(s?)://', '', location_url).split('/', 1)
          new_host = new_parts[0]
          new_resource = '/' + new_parts[1] if len(new_parts) == 2 else '/'

          originServerSocket.close()
          originServerSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
          originServerSocket.connect((new_host, 80))

          new_request = f"GET {new_resource} HTTP/1.1\r\nHost: {new_host}\r\nConnection: close\r\n\r\n"
          originServerSocket.sendall(new_request.encode())
          response_data = originServerSocket.recv(BUFFER_SIZE)


      #try:
        #originServerSocket.sendall(request.encode())
      #except socket.error:
       # print ('Forward request to origin failed')
        #sys.exit()

      #print('Request sent to origin server\n')

      # Get the response from the origin server
      # ~~~~ INSERT CODE ~~~~

      clientSocket.sendall(response_data)

      # ~~~~ END CODE INSERT ~~~~

      # Send the response to the client
      # ~~~~ INSERT CODE ~~~~

      # ~~~~ END CODE INSERT ~~~~

      # Create a new file in the cache for the requested file.
      cacheDir, file = os.path.split(cacheLocation)
      print ('cached directory ' + cacheDir)
      if not os.path.exists(cacheDir):
        os.makedirs(cacheDir)
        with open(cacheLocation, 'wb') as cacheFile:
          cacheFile.write(response_data)
        print('Response saved to cache')

      # Save origin server response in the cache file
      # ~~~~ INSERT CODE ~~~~
      #cacheFile.write(response)
  
      # ~~~~ END CODE INSERT ~~~~
      #cacheFile.close()
      #print ('cache file closed')

      # finished communicating with origin server - shutdown socket writes
      #print ('origin response received. Closing sockets')
      originServerSocket.close()
       
      clientSocket.shutdown(socket.SHUT_WR)
      print ('client socket shutdown for writing')
    except OSError as err:
      print ('origin server request failed. ' + err.strerror)

  try:
    clientSocket.close()
  except:
    print ('Failed to close client socket')
