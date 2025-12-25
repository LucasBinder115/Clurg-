#!/bin/bash
gdb -batch -ex "run" -ex "bt" ./bin/clurg-web &
sleep 2
curl -s http://localhost:8080/api/v1/system/status > /dev/null
sleep 1
kill %1 2>/dev/null
wait
