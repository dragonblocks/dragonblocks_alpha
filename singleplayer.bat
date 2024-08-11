@echo off
start "Internal Server" dragonblocks-server.exe "[::1]:4000"
dragonblocks-client.exe singleplayer "[::1]:4000"
taskkill /FI "Internal Server" /T /F
