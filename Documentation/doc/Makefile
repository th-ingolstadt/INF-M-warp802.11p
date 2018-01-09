## Markdown extension (e.g. md, markdown, mdown).
MEXT = md

## All markdown files in the working directory
SRC = $(wildcard *.$(MEXT))


PDFS=$(SRC:.md=.pdf)
HTML=$(SRC:.md=.html)
TEX=$(SRC:.md=.tex)
# DOCX=$(SRC:.md=.docx)


all:	$(PDFS) $(HTML) $(TEX)

pdf:	clean $(PDFS)
html:	clean $(HTML)
tex:	clean $(TEX)
# docx:	clean $(DOCX)

%.html:	%.md
	pandoc -r markdown+simple_tables+table_captions+yaml_metadata_block+abbreviations -w html -S --filter pandoc-fignos --filter pandoc-tablenos --filter pandoc-citeproc -o $@ $<

%.tex:	%.md
	pandoc -r markdown+simple_tables+table_captions+yaml_metadata_block+abbreviations -w latex -s -S --latex-engine=xelatex --biblatex --csl ieee.csl --filter pandoc-fignos --filter pandoc-tablenos -o $@ $<

%.pdf:	%.md
	pandoc -r markdown+simple_tables+table_captions+yaml_metadata_block+abbreviations -s -S --latex-engine=xelatex --biblatex --csl ieee.csl --filter pandoc-fignos --filter pandoc-tablenos -o $@ $<

#%.docx: %.md
#	pandoc -r markdown+simple_tables+table_captions+yaml_metadata_block+abbreviations -w docx -S  --filter pandoc-fignos --filter pandoc-tablenos --filter pandoc-citeproc -o $@ $<

clean:
	rm -f *.html *.pdf *.tex *.docx
