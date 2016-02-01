#!/bin/bash

./ftps &
IP="127.0.0.1"
PORT="1040"
./ftpc $IP $PORT qwertyuiopasdfghjklz
