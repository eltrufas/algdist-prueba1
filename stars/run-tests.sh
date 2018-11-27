#!/usr/bin/env bash

source vars.sh

get_times() {
  read a
  read b
  echo $a | grep "wall time" | awk '{print $3}'
  echo $b | grep "cpu time" | awk '{print $3}'
}

# eliminar los resultados de corridas anteriores
rm -rf results
mkdir results

# Escribir cabecera de los archivos de resultados
header=n,row-seq,col-seq

for p in row-par col-par; do
  for n in $THREADS; do 
    echo $n
    header=$header,$p-$n
  done
done

echo $header > results/wall-time.csv
echo $header > results/cpu-time.csv

# Correr pruebas
for i in $(seq 1 $N_REPS); do 
  # Correr N_REPS por tamaño de problema, con intención de tomar el valor mediano.
  echo Repeticion $i
  for f in ./data/*; do
    dim=`head -n1 $f`
    echo Corriendo para datos con dimension $dim
    row_wall=$dim
    row_cpu=$dim

    # Version secuencial por fila
    read wall cpu <<< `./bin/stars-row -S < $f | get_times`
    row_wall=$row_wall,$wall
    row_cpu=$row_cpu,$cpu

    # Version secuencial por columna
    read wall cpu <<< `./bin/stars-col -S < $f | get_times`
    row_wall=$row_wall,$wall
    row_cpu=$row_cpu,$cpu

    # Correr varias veces por cada combinaci'on de binario y numero de hilos
    for p in row col; do 
      for n in $THREADS; do 
        read wall cpu <<< `./bin/stars-$p-par -S $n < $f | get_times`
        row_wall=$row_wall,$wall
        row_cpu=$row_cpu,$cpu
      done
    done

    echo $row_wall >> results/wall-time.csv
    echo $row_cpu >> results/cpu-time.csv
    
  done
done
