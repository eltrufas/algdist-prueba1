#!/usr/bin/env bash

source vars.sh

rm -rf data
mkdir -p data

for i in `seq $MIN_DATA_POINTS $MAX_DATA_POINTS`; do
  n=`echo "1000*$i" | bc`
  echo Generando ejemplo de tamaño $n
  ./bin/generate-stars $n > data/$n  
done

