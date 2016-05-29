#!/bin/bash

for i in {1..20}
do
	echo "client $i"
	./client_single $i &

done
