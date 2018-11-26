#!/usr/bin/env bash
# [wf] execute run stage

get_times() {
  read a
  read b
  echo $a | grep "wall time" | awk '{print $3}'
}

rm -rf results
mkdir results

echo n,time > results/seq.csv
echo n,threads,time > results/v2.csv
echo n,threads,time > results/v3.csv

for i in $(seq 1 $N_REPS); do
  echo Iniciando repetición $i
  for f in ./data/*; do
    dim=`head -n1 $f`
    Corriendo para datos con dimensión $dim

    # Version secuencial
    read wall cpu <<< `./bin/seq-squares $i | get_times`
    echo $i,wall >> results/seq.csv

    # Correr varias veces por cada combinaci'on de binario y numero de hilos
    for n in $THREADS; do 
      read wall cpu <<< `./bin/par-squares-g-v2 $i $n | get_times`
      echo $i,$n,$wall >> results/$p.csv

      read wall cpu <<< `./bin/par-squares-v2 $i $n | get_times`
      echo $i,$n,$wall >> results/$p.csv
    done
  done
done


for n in $THREADS; do
  
done
