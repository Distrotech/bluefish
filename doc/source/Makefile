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
BOOK_FO_TARGET=${OUTPUT_DIR}/book.fo
BOOK_PS_TARGET=${OUTPUT_DIR}/book.ps
BOOK_FO_TARGET=${OUTPUT_DIR}/book.fo
BOOK_XML_SOURCE=${BOOK_DIR}/book.xml

XSL_DIR=${TOOLS_DIR}/tools/xsl
XSL_FO=${XSL_DIR}/fo/docbook.xsl
XSL_HTML=${TOOLS_DIR}/db2longhtml.xsl
XSL_HTML_CHUNK=${TOOLS_DIR}/db2html.xsl

RUN_FOP=${TOOLS_DIR}/tools/bin/run-fop.sh
RUN_XALAN=${TOOLS_DIR}/tools/bin/run-xalan.sh

all: html pdf ps

define move-pictures
mkdir -p $(OUTPUT_FIG)
mkdir -p $(OUTPUT_IMG)
cp $(SOURCE_FIG)/*.png $(OUTPUT_FIG)/ 
cp $(SOURCE_IMG)/*.png $(OUTPUT_IMG)/ 
endef

html: clean-html validate-all
	$(XSLTPROC) -output $(OUTPUT_DIR)/ $(XSL_HTML_CHUNK) $(BOOK_XML_SOURCE)
	$(move-pictures)
	
html-one-big: clean-html validate-all
	$(XSLTPROC) -output $(BOOK_HTML_TARGET) $(XSL_HTML) $(BOOK_XML_SOURCE)
	$(move-pictures)

pdf: clean-pdf validate-all
	$(RUN_FOP) $(BOOK_TOP)  -xml $(BOOK_XML_SOURCE) -xsl $(XSL_FO) -pdf $(BOOK_PDF_TARGET)

fo: clean-fo validate-all
	$(RUN_XALAN) -in $(BOOK_XML_SOURCE) -xsl $(XSL_FO) -out $(BOOK_FO_TARGET)

ps: clean-ps validate-all
	$(RUN_FOP) $(BOOK_TOP) -xml $(BOOK_XML_SOURCE) -xsl $(XSL_FO) -ps $(BOOK_PS_TARGET)

validate-all:
	xmllint --noout --dtdvalid $(TOOLS_DIR)/tools/dtd/dblite.dtd $(BOOK_XML_SOURCE)

clean-html:
	rm -f $(OUTPUT_DIR)/*.html
	rm -rf $(OUTPUT_FIG)
	rm -rf $(OUTPUT_IMG)

clean-pdf:
	rm -f $(BOOK_PDF_TARGET)

clean-fo:
	rm -f $(BOOK_FO_TARGET)
 
clean-ps:
	rm -f $(BOOK_PS_TARGET) 

clean-all: clean-html clean-ps clean-pdf clean-fo
