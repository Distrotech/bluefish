XSLTPROC = xsltproc

## You shouldn't normally need to edit anything below here.
SHELL = /bin/sh

BOOK_TOP=.
BOOK_DIR=${BOOK_TOP}
OUTPUT_DIR=..
SOURCE_FIG=${BOOK_DIR}/figures
OUTPUT_FIG=${OUTPUT_DIR}/figures
SOURCE_IMG=${BOOK_DIR}/imgs
OUTPUT_IMG=${OUTPUT_DIR}/imgs
TOOLS_DIR=../../../bluefish-doctools
BOOK_HTML_TARGET=${OUTPUT_DIR}/book.html
BOOK_FO_TARGET=${OUTPUT_DIR}/book.fo
BOOK_XML_A4= ${TOOLS_DIR}/titlepage-a4.xml
BOOK_XSL_A4= ${TOOLS_DIR}/titlepage-a4.xsl
BOOK_XML_US= ${TOOLS_DIR}/titlepage-us.xml
BOOK_XSL_US= ${TOOLS_DIR}/titlepage-us.xsl
BOOK_LAYER_A4=${TOOLS_DIR}/pdf-ps-a4.xsl
BOOK_LAYER_US=${TOOLS_DIR}/pdf-ps-us.xsl
BOOK_TEMPLATE= ${XSL_DIR}/template/titlepage.xsl
BOOK_PDF_A4_TARGET=${OUTPUT_DIR}/book-a4.pdf
BOOK_PDF_US_TARGET=${OUTPUT_DIR}/book-us.pdf
BOOK_PS_A4_TARGET=${OUTPUT_DIR}/book-a4.ps
BOOK_PS_US_TARGET=${OUTPUT_DIR}/book-us.ps
BOOK_XML_SOURCE=${BOOK_DIR}/book.xml

XSL_DIR=${TOOLS_DIR}/tools/xsl
XSL_FO=${XSL_DIR}/fo/docbook.xsl
XSL_HTML_CHUNK=${TOOLS_DIR}/db2html.xsl

RUN_FOP=${TOOLS_DIR}/tools/bin/run-fop.sh

define move-pictures
mkdir -p ${OUTPUT_FIG}
mkdir -p ${OUTPUT_IMG}
cp ${SOURCE_FIG}/*.png ${OUTPUT_FIG}/ 
cp ${SOURCE_IMG}/*.png ${OUTPUT_IMG}/ 
endef

all: html pspdf-a4 pspdf-us

all-a4: html pspdf-a4

all-us: html pspdf-us

html: clean-html  validate-all
	${XSLTPROC} -output ${OUTPUT_DIR}/ ${XSL_HTML_CHUNK} ${BOOK_XML_SOURCE}
	${move-pictures}
	
pdf-a4: clean-pdf-a4 validate-all fo-a4
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -pdf ${BOOK_PDF_A4_TARGET}
	rm -f ${BOOK_FO_TARGET}

pdf-us: clean-pdf-us validate-all fo-us
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -pdf ${BOOK_PDF_US_TARGET}
	rm -f ${BOOK_FO_TARGET}

ps-a4: clean-ps-a4 validate-all fo-a4
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -ps ${BOOK_PS_A4_TARGET}
	rm -f ${BOOK_FO_TARGET}

ps-us: clean-ps-us validate-all fo-us
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -ps ${BOOK_PS_US_TARGET}
	rm -f ${BOOK_FO_TARGET}

pspdf-a4: clean-pspdf-a4 validate-all fo-a4
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -pdf ${BOOK_PDF_A4_TARGET}
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -ps ${BOOK_PS_A4_TARGET}
	rm -f ${BOOK_FO_TARGET}
	
pspdf-us: clean-pspdf-us validate-all fo-us
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -pdf ${BOOK_PDF_US_TARGET}
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -ps ${BOOK_PS_US_TARGET}
	rm -f ${BOOK_FO_TARGET}
	
fo-a4: 
	${XSLTPROC} -output ${BOOK_XSL_A4} ${BOOK_TEMPLATE} ${BOOK_XML_A4}
	${XSLTPROC} -output ${BOOK_FO_TARGET} ${BOOK_LAYER_A4} ${BOOK_XML_SOURCE}

fo-us:
	${XSLTPROC} -output ${BOOK_XSL_US} ${BOOK_TEMPLATE} ${BOOK_XML_US}
	${XSLTPROC} -output ${BOOK_FO_TARGET} ${BOOK_LAYER_US} ${BOOK_XML_SOURCE}

validate-all:
	xmllint --noout --valid ${BOOK_XML_SOURCE}

clean-html:
	rm -f ${OUTPUT_DIR}/*.html
	rm -rf ${OUTPUT_FIG}
	rm -rf ${OUTPUT_IMG}

clean-fo:
	rm -f ${BOOK_FO_TARGET}
	
clean-xsl-a4:
		rm -f ${BOOK_XSL_A4}
		
clean-xsl-us:
	rm -f ${BOOK_XSL_US}

clean-pdf-a4: clean-fo clean-xsl-a4
	rm -f ${BOOK_PDF_A4_TARGET}
	
clean-pdf-us: clean-fo clean-xsl-us
	rm -f ${BOOK_PDF_US_TARGET}

clean-ps-a4: clean-fo clean-xsl-a4
	rm -f ${BOOK_PS_A4_TARGET}

clean-ps-us:  clean-fo clean-xsl-us
	rm -f ${BOOK_PS_US_TARGET}
	
clean-pspdf-a4: clean-fo clean-xsl-a4
	rm -f ${BOOK_PS_A4_TARGET}
	rm -f ${BOOK_PDF_A4_TARGET}

clean-pspdf-us: clean-fo clean-xsl-us
	rm -f ${BOOK_PS_US_TARGET}
	rm -f ${BOOK_PDF_US_TARGET}

clean-all-a4: clean-html clean-pspdf-a4

clean-all-us: clean-html clean-pspdf-us

clean-all: clean-html clean-pspdf-a4 clean-pspdf-us

## Obsolete
## XSL_HTML=${TOOLS_DIR}/db2longhtml.xsl

## RUN_XALAN=${TOOLS_DIR}/tools/bin/run-xalan.sh

## html-one-big: clean-html validate-all
##	${XSLTPROC} -output ${BOOK_HTML_TARGET} ${XSL_HTML} ${BOOK_XML_SOURCE}
##	${move-pictures}
