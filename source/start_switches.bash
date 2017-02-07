#!/bin/bash
clear
echo "Starting switches, pids will be printed..."

C_PORT="3002"
C_HOST="ecegrid-thin7"

echo "#!/bin/bash" > killproc.bash
chmod +x killproc.bash

./switch_sdn 0 $C_HOST $C_PORT -l &
echo "kill $!" >> killproc.bash
sleep 1
./switch_sdn 1 $C_HOST $C_PORT -l &
echo "kill $!" >> killproc.bash
sleep 1
./switch_sdn 2 $C_HOST $C_PORT -l &
echo "kill $!" >> killproc.bash
sleep 1
./switch_sdn 3 $C_HOST $C_PORT -l &
echo "kill $!" >> killproc.bash
sleep 1
./switch_sdn 4 $C_HOST $C_PORT -l &
echo "kill $!" >> killproc.bash
sleep 1
./switch_sdn 5 $C_HOST $C_PORT -l &
echo "kill $!" >> killproc.bash

echo "Switches started..."
