#!/usr/bin/env bash
# [wf] execute run stage

source vars.sh

get_times() {
  read a
  read b
  echo $a | grep "wall time" | awk '{print $3}'
  echo $b | grep "cpu time" | awk '{print $3}'
}

rm -rf results
mkdir results

echo n,time > results/seq.csv
echo n,threads,time > results/par.csv

for i in $(seq 1 $N_REPS); do
  echo Iniciando repetición $i
  for s in $(seq $MIN_DATA_POINTS $MAX_DATA_POINTS); do
    dim=$s
    echo Corriendo para datos con dimensión $dim

    # Version secuencial
    read wall cpu <<< `./bin/seq -S $dim | get_times`
    echo $dim,$wall >> results/seq.csv

    # Correr varias veces por cada combinaci'on de binario y numero de hilos
    for n in $THREADS; do 
      read wall cpu <<< `./bin/par -S $dim $n | get_times`
      echo $dim,$n,$wall >> results/par.csv

    done
  done
done


