#!/bin/bash

array=( "make" "g++" "python3" )
for i in "${array[@]}"
do
    command -v $i >/dev/null 2>&1 || { 
        echo >&2 "$i required"; 
        exit 1; 
    }
done

pip3 list | grep matplotlib >/dev/null 2>&1 || {
   echo >&2 python3.matplotlib required
   exit 1; 
}

echo Good to go!
