#!/bin/bash

IRC_SERVER_IP="127.0.0.1"
IRC_SERVER_PORT="8888"
IRC_PASSWORD="8"
MAX_USERS=1000
CHANNEL="#testchannel"

irc_client_simulator() {
  local client_id=$1
  local nick="user$client_id"
  local user="user$client_id 0 * :Test User"

  {
    printf "PASS %s\r\n" "$IRC_PASSWORD"
    printf "NICK %s\r\n" "$nick"
    printf "USER %s\r\n" "$user"
    printf "JOIN %s\r\n" "$CHANNEL"

    for i in {1..5}; do
      printf "PRIVMSG %s :Hello from %s %d\r\n" "$CHANNEL" "$nick" "$i"
      sleep 0.1
    done
  } | nc $IRC_SERVER_IP $IRC_SERVER_PORT
}

for i in $(seq 1 $MAX_USERS); do
  irc_client_simulator $i &
done

wait

echo "✅ Stress test completed with $MAX_USERS clients."
