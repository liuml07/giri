# GNU/Linux Makefile for the slicing

.PHONY: all clean clobber help

all: main.pdf clean

main.pdf: $(shell ls *.tex) references.bib
	pdflatex main
	bibtex main
	pdflatex main
	pdflatex main

clean:
	@ rm -rf *.aux *.bbl *.blg *.log *.out

clobber: clean
	@ rm -rf main.pdf ../figures/*-eps-converted-to.pdf

rebuild: clobber all

help:
	@ echo "God helps those who help themselves."
