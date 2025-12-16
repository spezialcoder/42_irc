#!/bin/bash
# Simple IRC client wrapper that adds CRLF for proper IRC protocol
# Usage: ./irc_client.sh [host] [port]

HOST=${1:-localhost}
PORT=${2:-7850}

echo "Connecting to IRC server at $HOST:$PORT"
echo "Type your messages and press Enter."
echo "---"

# Simple: read lines, add CRLF, pipe to netcat
while IFS= read -r line; do
  printf "%s\r\n" "$line"
done | nc "$HOST" "$PORT"
