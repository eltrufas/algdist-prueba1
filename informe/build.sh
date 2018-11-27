#!/usr/bin/env bash
# [wf] execute build stage.

rm -f images/*

cp ../squares/charts/* images/
cp ../stars/charts/* images/
cp ../powerset/charts/* images/

pdflatex informe.tex
