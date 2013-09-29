all: main.pdf clean

main.pdf: main.tex references.bib
	pdflatex main
	bibtex main
	pdflatex main
	pdflatex main

clean:
	@rm -rf main.aux *.log main.out *.bbl *.blg

clobber: clean
	@rm -rf main.pdf

rebuild: clobber all
