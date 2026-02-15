#!/usr/bin/env python3
import socket
import time

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect(('157.90.114.127', 6666))

# Send "PASS abc\r\n" in fragments
sock.send(b"PAS")
time.sleep(0.1)
sock.send(b"S a")
time.sleep(0.1)
sock.send(b"bc\r\n")

time.sleep(0.5)
sock.settimeout(1)
try:
    print(sock.recv(4096).decode())
except:
    pass
sock.close()
