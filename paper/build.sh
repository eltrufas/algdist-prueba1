#!/usr/bin/env bash
# [wf] execute build stage.

rm -f images/*

cp ../pipelines/squares/charts/* images/
cp ../pipelines/stars/charts/* images/

pdflatex informe.tex
