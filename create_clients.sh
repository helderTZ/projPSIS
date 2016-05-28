#!/bin/bash

for i in {1..2}
do
	echo "client $i"
	./client_single &

done
