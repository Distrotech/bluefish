XSLTPROC = xsltproc

## You shouldn't normally need to edit anything below here.
SHELL = /bin/sh

BOOK_TOP=.
BOOK_DIR=${BOOK_TOP}
OUTPUT_DIR=..
OUTPUT_FIG=$(OUTPUT_DIR)/figures
SOURCE_FIG=$(BOOK_DIR)/figures
OUTPUT_IMG=$(OUTPUT_DIR)/imgs
SOURCE_IMG=$(BOOK_DIR)/imgs
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

define move-html
mv *.html $(OUTPUT_DIR)/
endef

define move-pictures
mkdir -p $(OUTPUT_FIG)
mkdir -p $(OUTPUT_IMG)
cp $(SOURCE_FIG)/*.png $(OUTPUT_FIG)/ 
cp $(SOURCE_IMG)/*.png $(OUTPUT_IMG)/ 
endef

html: clean-html $(BOOK_ALL_SOURCE)
	$(XSLTPROC) $(XSL_HTML_CHUNK) $(BOOK_XML_SOURCE) > $(BOOK_HTML_TARGET)
	$(move-html)
	$(move-pictures)
	
html-one-big: clean-html $(BOOK_ALL_SOURCE)
	$(XSLTPROC) $(XSL_HTML) $(BOOK_XML_SOURCE) > $(BOOK_HTML_TARGET)
	$(move-pictures)

clean-html:
	rm -f $(OUTPUT_DIR)/*.html
	rm -rf $(OUTPUT_FIG)
	rm -rf $(OUTPUT_IMG)

clean-pdf:
	rm -f $(BOOK_PDF_TARGET)

pdf: clean-pdf $(BOOK_ALL_SOURCE) 
	$(RUN_FOP) $(BOOK_TOP)  -xml $(BOOK_XML_SOURCE) -xsl $(XSL_FO) -pdf $(BOOK_PDF_TARGET)
	
ps: clean-ps $(BOOK_ALL_SOURCE)	
	$(RUN_FOP) $(BOOK_TOP) -xml $(BOOK_XML_SOURCE) -xsl $(XSL_FO) -ps $(BOOK_PS_TARGET)

clean-ps:
	rm -f $(BOOK_PS_TARGET) 

clean-all: clean-html clean-ps clean-pdf
