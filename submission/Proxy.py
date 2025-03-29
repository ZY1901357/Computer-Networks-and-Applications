
# Include the libraries for socket and system calls
import socket
import sys
import os
import argparse
import re
import time


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
  serverSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  # ~~~~ END CODE INSERT ~~~~
  print ('Created socket')
except:
  print ('Failed to create socket')
  sys.exit()

try:
  # Bind the the server socket to a host and port
  # ~~~~ INSERT CODE ~~~~
  serverSocket.bind((proxyHost, proxyPort))
  # ~~~~ END CODE INSERT ~~~~
  print ('Port is bound')
except:
  print('Port is already in use')
  sys.exit()

try:
  # Listen on the server socket
  # ~~~~ INSERT CODE ~~~~
  serverSocket.listen(5)
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
    clientSocket, clientAddr = serverSocket.accept() 
    print("THank you , now we have a connection !")
    # ~~~~ END CODE INSERT ~~~~
    print ('Received a connection')
  except:
    print ('Failed to accept connection')
    sys.exit()

  # Get HTTP request from client
  # and store it in the variable: message_bytes
  # ~~~~ INSERT CODE ~~~~
  #clientSocket, clientAddr = serverSocket.accept() 
  # ~~~~ END CODE INSERT ~~~~
  message_bytes = clientSocket.recv(BUFFER_SIZE)
  message = message_bytes.decode('utf-8')
  print ('Received request:')
  print ('< ' + message)

  # Extract the method, URI and version of the HTTP client request 
  requestParts = message.split()
  if len(requestParts) < 3:
    print("Incomplete or invalid HTTP request received. Closing connection.")
    clientSocket.close()
    continue

  method = requestParts[0]
  URI = requestParts[1]
  version = requestParts[2]

  print ('Method:\t\t' + method)
  print ('URI:\t\t' + URI)
  print ('Version:\t' + version)
  print ('')

  if len(requestParts) < 3:
    print("Incomplete or invalid HTTP request received. Closing connection.")
    clientSocket.close()
    continue

  # Get the requested resource from URI
  # Remove http protocol from the URI
  URI = re.sub('^(/?)http(s?)://', '', URI, count=1)

  # Remove parent directory changes - security
  URI = URI.replace('/..', '')

  # Split hostname from resource name
  resourceParts = URI.split('/', 1)
  hostname = resourceParts[0]
  resource = '/'

  if len(resourceParts) == 2:
    # Resource is absolute URI with hostname and resource
    resource = resource + resourceParts[1]

  print ('Requested Resource:\t' + resource)

  # Check if resource is in cache
  try:
    cacheLocation = './' + hostname + resource
    if cacheLocation.endswith('/'):
        cacheLocation = cacheLocation + 'default'

    print ('Cache location:\t\t' + cacheLocation)

    fileExists = os.path.isfile(cacheLocation)
    
    # Check wether the file is currently in the cache
    cache_valid = True
    timestamp_path = cacheLocation + '.time'

    # Check if cached response has expired or not
    if os.path.isfile(timestamp_path):
        with open(timestamp_path, 'r') as timeFile:
            cached_time, max_age = map(int, timeFile.read().split(','))
        current_time = int(time.time())
        if current_time - cached_time > max_age:
            print("Cached file expired. Fetching fresh data.")
            cache_valid = False
            os.remove(cacheLocation)
            os.remove(timestamp_path)

    if cache_valid:
        with open(cacheLocation, "rb") as cacheFile:
            cacheData = cacheFile.read()
        clientSocket.sendall(cacheData)
        print ('Cache hit! Loaded fresh from cache: ' + cacheLocation)
        continue  # important to skip fetching again


    print ('Cache hit! Loading from cache file: ' + cacheLocation)
    # ProxyServer finds a cache hit
    # Send back response to client 
    # ~~~~ INSERT CODE ~~~~
    ##clientSocket.sendall("".join(cacheData).encode()) 
    # ~~~~ END CODE INSERT ~~~~
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
      originServerRequestHeader = f"Host: {hostname}\r\nConnection: close\r\n"
      # ~~~~ END CODE INSERT ~~~~

      # Construct the request to send to the origin server
      request = originServerRequest + '\r\n' + originServerRequestHeader + '\r\n\r\n'
      originServerSocket.sendall(request.encode())
      print ('Connected to origin Server')

      # Request the web resource from origin server
      print ('Forwarding request to origin server:')
      for line in request.split('\r\n'):
        print ('> ' + line)

      #try:
        #originServerSocket.sendall(request.encode())
      #except socket.error:
        #print ('Forward request to origin failed')
        #sys.exit()

      print('Request sent to origin server\n')

      # Get the response from the origin server
      # ~~~~ INSERT CODE ~~~~
      response_data = originServerSocket.recv(BUFFER_SIZE)
      
      # ~~~~ END CODE INSERT ~~~~

      # Send the response to the client
      # ~~~~ INSERT CODE ~~~~
      clientSocket.sendall(response_data)
      # ~~~~ END CODE INSERT ~~~~

      # Create a new file in the cache for the requested file.
      cacheDir, file = os.path.split(cacheLocation)
      

      # Save origin server response in the cache file
      # ~~~~ INSERT CODE ~~~~
      print ('cached directory ' + cacheDir)
      if not os.path.exists(cacheDir):
        os.makedirs(cacheDir)
      with open(cacheLocation, 'wb') as cacheFile:
      # ~~~~ END CODE INSERT ~~~~
        cacheFile.write(response_data)
      response_text = response_data.decode('utf-8', 'ignore')
      headers, _, body = response_text.partition('\r\n\r\n')

      max_age_seconds = None
      for line in headers.split("\r\n"):
          if line.lower().startswith("cache-control:"):
              match = re.search(r'max-age=(\d+)', line, re.I)
              if match:
                  max_age_seconds = int(match.group(1))
                  print(f"Found max-age: {max_age_seconds} seconds")
                  break

      if max_age_seconds is not None:
          with open(cacheLocation + '.time', 'w') as timeFile:
              timeFile.write(str(int(time.time())) + ',' + str(max_age_seconds))

      print('Response saved to cache')

      originServerSocket.close()
      clientSocket.shutdown(socket.SHUT_WR)

  
      # ~~~~ END CODE INSERT ~~~~
      #cacheFile.close()
      print ('cache file closed')

      # finished communicating with origin server - shutdown socket writes
      print ('origin response received. Closing sockets')
      originServerSocket.close()
       
      clientSocket.shutdown(socket.SHUT_WR)
      print ('client socket shutdown for writing')
    except OSError as err:
      print ('origin server request failed. ' + err.strerror)

  try:
    clientSocket.close()
  except:
    print ('Failed to close client socket')
