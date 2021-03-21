#! /bin/bash

# 		  AUTH      Name          SETNODE   x = 0   y = 0   z = 0   DIRT 	  GETBLOCK  x = 0   y = 0   z = 0   DISCONNECT
echo -ne "\0\0\0\x02Fleckenstein\0\0\0\0\x04\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x04\0\0\0\x03\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x01" | nc localhost 4000 | hexdump
