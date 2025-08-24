#!/bin/bash

SERVER_CMD="./ircserver 6667 password"  # Change this if your server takes different args

echo "🔄 Stopping old IRC server..."
pkill -f "ircserver"   # Kills old server processes

echo "🔨 Recompiling..."
if make; then
    echo "✅ Build successful. Starting server..."
    $SERVER_CMD &
    SERVER_PID=$!
    echo "📡 Server started with PID $SERVER_PID"
else
    echo "❌ Build failed. Fix errors and try again."
    exit 1
fi

# Reconnect all irssi sessions using their FIFO
for fifo in $(find /tmp -name 'irssi*' -type s); do
    echo "🔁 Reconnecting irssi via FIFO: $fifo"
    echo "/reconnect" > "$fifo"
done

echo "✅ Done! Server rebuilt and all irssi clients reconnected."
