@echo off
start "Internal Server" dragonblocks_server.exe "[::1]:4000"
echo "singleplayer" | dragonblocks_client.exe "[::1]:4000"
taskkill /FI "Internal Server" /T /F

