#!/usr/bin/env python3
"""
Simple IRC client that sends proper CRLF line endings
Usage: python3 irc_client.py [host] [port]
"""

import socket
import sys
import select

def main():
    host = sys.argv[1] if len(sys.argv) > 1 else 'localhost'
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 7850
    
    print(f"Connecting to IRC server at {host}:{port}")
    print("Type your messages and press Enter. Press Ctrl+C to exit.")
    print("---")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))
        sock.setblocking(False)
        
        while True:
            # Check for data from server
            ready = select.select([sock, sys.stdin], [], [], 0.1)
            
            if sock in ready[0]:
                data = sock.recv(4096)
                if not data:
                    print("\n[Disconnected from server]")
                    break
                print(data.decode('utf-8', errors='ignore'), end='', flush=True)
            
            if sys.stdin in ready[0]:
                line = sys.stdin.readline()
                if line:
                    # Send with CRLF (IRC standard)
                    sock.sendall((line.rstrip('\n') + '\r\n').encode('utf-8'))
                    
    except KeyboardInterrupt:
        print("\n[Disconnected]")
    except Exception as e:
        print(f"\n[Error: {e}]")
    finally:
        sock.close()

if __name__ == '__main__':
    main()
