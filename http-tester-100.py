#!/usr/bin/env python

from threading import Thread
from httplib import HTTPConnection
from BaseHTTPServer import BaseHTTPRequestHandler,HTTPServer
from datetime import datetime, timedelta
#from bcolor import bcolors
import sys
import time

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'

    def disable(self):
        self.HEADER = ''
        self.OKBLUE = ''
        self.OKGREEN = ''
        self.WARNING = ''
        self.FAIL = ''
        self.ENDC = ''

class TestHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        if self.path == "/basic":
            cdata = open("./basic", "r").read()
        if self.path == "/basic2":
            cdata = open("./basic2", "r").read()
        if self.path == "/basic3":
            cdata = open("./basic3", "r").read()
            #time.sleep(1)
        if self.path == "/cacheTest":
            cdata = str(time.time())

        size = len(cdata)
        expireDate=(datetime.now()+timedelta(days=1)).strftime("%a, %d %b %Y %H:%M:%S GMT")
        lastModify=(datetime.now()+timedelta(days=-1)).strftime("%a, %d %b %Y %H:%M:%S GMT")
        self.send_response(200)
        self.send_header('Content-Type','text/html')
        self.send_header('Content-Length', str(size))
        self.send_header('Expires',expireDate)
        self.send_header('Last-Modified', lastModify)
        if self.close_connection == True:
            self.send_header('Connection', 'close')
        self.end_headers()
        self.wfile.write(cdata)

        return

class ServerThread (Thread):
    def __init__(self, port):
        Thread.__init__(self)
        self.port = port

    def run(self):
        try:
            TestHandler.protocol_version = "HTTP/1.1"
            self.server = HTTPServer(('', self.port), TestHandler)
            self.server.serve_forever()
        except KeyboardInterrupt:
            self.server.socket.close()                                                                                                                                                       
    
class ClientThread (Thread):
    def __init__(self, proxy, url, file):
        Thread.__init__(self)
        self.proxy = proxy
        self.url = url
        self.file = file
        self.result = False
        self.data = ""

    def run(self):

        if self.file:
            dataFile = open(self.file, "r")
            cdata = dataFile.read()
        
            conn = HTTPConnection(self.proxy)
            conn.request("GET", self.url)
            resp = conn.getresponse()
            rdata = resp.read()

            if rdata == cdata:
                self.result = True
            self.data = rdata
            conn.close()
            dataFile.close()
        else:
            conn = HTTPConnection(self.proxy)
            conn.request("GET", self.url)
            resp = conn.getresponse()
            rdata = resp.read()
            
            if resp.status == httplib.OK:
                self.result = True
            conn.close()

class ClientPersistThread(Thread):
    def __init__(self, proxy, url, file, url2, file2):
        Thread.__init__(self)
        self.proxy = proxy
        self.url = url
        self.file = file
        self.url2 = url2
        self.file2 = file2
        self.result = False

    def run(self):
        conn = HTTPConnection(self.proxy)
        tmpFlag = True

        dataFile = open(self.file, "r")
        cdata = dataFile.read()
        dataFile = open(self.file2, "r")
        cdata2 = dataFile.read()

        conn.request("GET", self.url)
        resp = conn.getresponse()
        rdata = resp.read()
        if rdata != cdata:
            tmpFlag = False
            
        if resp.will_close == True:
            tmpFlag = False

        connHdrs = {"Connection": "close"}
        conn.request("GET", self.url2, headers=connHdrs)

        resp = conn.getresponse()
        rdata2 = resp.read()
        if rdata2 != cdata2:
            tmpFlag = False

        if resp.will_close == False:
            tmpFlag = False

        if tmpFlag == True:
            self.result = True
        conn.close()
        dataFile.close()


conf = open("./portconf", "r")
pport  = conf.readline().rstrip().split(':')[1]
sport1 = conf.readline().rstrip().split(':')[1]
sport2 = conf.readline().rstrip().split(':')[1]

b1 = open("./basic", "w")
b1.write("basic\n")
b2 = open("./basic2", "w")
b2.write("aloha\n")
b3 = open("./basic3", "w")
b3.write("cat\n")

b1.close()
b2.close()
b3.close()

servers = []
for i in range(150):
  servers.append(ServerThread(int(sport1)+i))
  servers[i].start()
  
clients = []

for i in range(15):
  print "Starting batch of 10 requests, starting with " + str(i*10)
  for j in range(10):
      #clients.append(ClientThread("127.0.0.1:" + pport, "http://urlecho.appspot.com/echo?status=200&Content-Type=text%2Fhtml&body=hello" + str(i), "./basic"))
      clients.append(ClientThread("127.0.0.1:" + pport, "http://127.0.0.1:"+ str(int(sport1)+i*10+j) +"/basic3", "./basic3"))
      clients[i*10+j].start()
      #clients.append(ClientThread("127.0.0.1:" + pport, "http://127.0.0.1:"+ sport2 +"/basic3", "./basic3"))   
  time.sleep(5)

#for c in clients:
#    c.start()

for c in clients:
    c.join()
    #print c.data

r = True
datafile = open("./basic3", "r")
cdata = datafile.read()
for c in clients:
    r = r and (c.data == cdata)

if r:
    print "20 Concurrent Connection: [" + bcolors.OKGREEN + "PASSED" + bcolors.ENDC + "]"
else:
    print "20 Concurrent Connection: [" + bcolors.FAIL + "FAILED" + bcolors.ENDC + "]"

print "All tests done. Now shutting down servers..."

for server in servers:
  server.server.shutdown()
