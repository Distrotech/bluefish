XSLTPROC = xsltproc

## You shouldn't normally need to edit anything below here.
SHELL = /bin/sh

BOOK_TOP=.
BOOK_DIR=${BOOK_TOP}
OUTPUT_DIR=..
TOOLS_DIR=../../../bluefish-doctools
BOOK_HTML_TARGET=${OUTPUT_DIR}/book.html
BOOK_PDF_TARGET=${OUTPUT_DIR}/book.pdf
BOOK_PS_TARGET=${OUTPUT_DIR}/book.ps
BOOK_FO_TARGET=${OUTPUT_DIR}/book.fo
BOOK_XML_SOURCE=${BOOK_DIR}/book.xml
BOOK_ALL_SOURCE=${BOOK_DIR}/*.xml

XSL_DIR=${TOOLS_DIR}/tools/xsl
XSL_FO=${XSL_DIR}/fo/docbook.xsl
XSL_HTML=${TOOLS_DIR}/db2longhtml.xsl
XSL_HTML_CHUNK=${TOOLS_DIR}/db2html.xsl

RUN_FOP=${TOOLS_DIR}/tools/bin/run-fop.sh

all: html pdf ps

html: $(BOOK_ALL_SOURCE)
	$(XSLTPROC) $(XSL_HTML_CHUNK) $(BOOK_XML_SOURCE) > $(BOOK_HTML_TARGET)
	mv *.html $(OUTPUT_DIR)/

html-one-big: $(BOOK_ALL_SOURCE)
	$(XSLTPROC) $(XSL_HTML) $(BOOK_XML_SOURCE) > $(BOOK_HTML_TARGET)

pdf: $(BOOK_ALL_SOURCE)
	$(XSLTPROC) $(XSL_FO) $(BOOK_XML_SOURCE) > $(BOOK_FO_TARGET)
	$(RUN_FOP) $(BOOK_TOP) -fo $(BOOK_FO_TARGET) -pdf $(BOOK_PDF_TARGET)

ps: $(BOOK_ALL_SOURCE)
	$(XSLTPROC) $(XSL_FO) $(BOOK_XML_SOURCE) > $(BOOK_FO_TARGET)
	$(RUN_FOP) $(BOOK_TOP) -fo $(BOOK_FO_TARGET) -ps $(BOOK_PS_TARGET)

clean:
	rm -f $(BOOK_HTML_TARGET) $(BOOK_FO_TARGET)
	rm -f $(BOOK_PDF_TARGET) $(BOOK_PS_TARGET) 
	rm -f $(BOOK_PDF_TARGET) $(BOOK_PS_TARGET) 
	rm -f $(OUTPUT_DIR)/*.html

