#!/bin/sh

res=1
while [ ${res} -ne 0 ]
do
  ./iridium-sender $1
  res=$?
  if [ ${res} -ne 0 ]
  then
    sleep 5
  fi
done

exit 0
