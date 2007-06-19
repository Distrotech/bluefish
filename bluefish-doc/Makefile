XSLTPROC = xsltproc --debug
HAVE_XSL_DIR=1

## You shouldn't normally need to edit anything below here.
SHELL = /bin/sh
VERSION=1.0-6
BOOK_PREFIX=bluefish-doc
BOOK_TOP=.
BOOK_DIR=${BOOK_TOP}/src
OUTPUT_DIR=${BOOK_TOP}/built-doc
SOURCE_FIG=${BOOK_DIR}/figures
OUTPUT_FIG=${OUTPUT_DIR}/figures
SOURCE_IMG=${BOOK_DIR}/imgs
OUTPUT_IMG=${OUTPUT_DIR}/imgs
STYLESHEETS_DIR=${BOOK_TOP}/stylesheets
TARBALL_DIR=${BOOK_TOP}/tarballs
BOOK_HTML_TARGET=${OUTPUT_DIR}/book.html
BOOK_FO_TARGET=${BOOK_DIR}/book.fo
BOOK_XML_A4=${STYLESHEETS_DIR}/titlepage-a4.xml
BOOK_XSL_A4=${STYLESHEETS_DIR}/titlepage-a4.xsl
BOOK_XML_US=${STYLESHEETS_DIR}/titlepage-us.xml
BOOK_XSL_US=${STYLESHEETS_DIR}/titlepage-us.xsl
BOOK_HTML_XML=${STYLESHEETS_DIR}/titlepage-html.xml
BOOK_HTML_XSL=${STYLESHEETS_DIR}/titlepage-html.xsl
BOOK_LAYER_A4=${STYLESHEETS_DIR}/pdf-ps-a4.xsl
BOOK_LAYER_US=${STYLESHEETS_DIR}/pdf-ps-us.xsl
BOOK_PDF_A4_TARGET=${OUTPUT_DIR}/book-a4.pdf
BOOK_PDF_US_TARGET=${OUTPUT_DIR}/book-us.pdf
BOOK_PS_A4_TARGET=${OUTPUT_DIR}/book-a4.ps
BOOK_PS_US_TARGET=${OUTPUT_DIR}/book-us.ps
BOOK_XML_SOURCE=${BOOK_DIR}/book.xml

XSL_DIR=http://docbook.sourceforge.net/release/xsl/current

HAVE_XSL_DIR=$(shell test -d docbook-xsl && echo "0")
ifeq (${HAVE_XSL_DIR},0)
XSL_DIR=./docbook-xsl
endif

XSL_FO=${XSL_DIR}/fo/docbook.xsl
BOOK_TEMPLATE=${XSL_DIR}/template/titlepage.xsl
XSL_HTML_CHUNK=${STYLESHEETS_DIR}/db2html.xsl
XSL_HTML=${STYLESHEETS_DIR}/db2longhtml.xsl

RUN_FOP=${BOOK_TOP}/run-fop.sh

define move-pictures
mkdir -p ${OUTPUT_FIG}
mkdir -p ${OUTPUT_IMG}
cp ${SOURCE_FIG}/*.png ${OUTPUT_FIG}/ 
cp ${SOURCE_IMG}/*.png ${OUTPUT_IMG}/ 
cp ${STYLESHEETS_DIR}/manual.css ${OUTPUT_DIR}/
endef

define reinit-html
if test "${HAVE_XSL_DIR}" = "0" ; then \
mv ${XSL_HTML_CHUNK} ${XSL_HTML_CHUNK}-orig; \
sed -e 's|..\/docbook-xsl|http\:\/\/docbook.sourceforge.net\/release\/xsl\/current|g' < ${XSL_HTML_CHUNK}-orig > ${XSL_HTML_CHUNK};\
rm -f ${XSL_HTML_CHUNK}-orig; \
fi
endef

define reinit-big-html
if test "${HAVE_XSL_DIR}" = "0" ; then \
mv ${XSL_HTML} ${XSL_HTML}-orig; \
sed -e 's|..\/docbook-xsl|http\:\/\/docbook.sourceforge.net\/release\/xsl\/current|g' < ${XSL_HTML}-orig > ${XSL_HTML}; \
rm -f ${XSL_HTML}-orig; \
fi
endef

define reinit-fo-a4
if test "${HAVE_XSL_DIR}" = "0" ; then \
mv ${BOOK_LAYER_A4} ${BOOK_LAYER_A4}-orig; \
sed -e 's|..\/docbook-xsl|http\:\/\/docbook.sourceforge.net\/release\/xsl\/current|g' < ${BOOK_LAYER_A4}-orig > ${BOOK_LAYER_A4}; \
rm -f ${BOOK_LAYER_A4}-orig; \
fi
endef

define reinit-fo-us
if test "${HAVE_XSL_DIR}" = "0" ; then \
mv ${BOOK_LAYER_US} ${BOOK_LAYER_US}-orig; \
sed -e 's|..\/docbook-xsl|http\:\/\/docbook.sourceforge.net\/release\/xsl\/current|g' < ${BOOK_LAYER_US}-orig > ${BOOK_LAYER_US}; \
rm -f ${BOOK_LAYER_US}-orig; \
fi
endef

define hackhtmltoc
mv ${OUTPUT_DIR}/bk01-toc.html ${OUTPUT_DIR}/bk01-toc-temp.html
sed -e 's|<h1>|<div class="book"><h1>|' \
		-e 's|<div class="navfooter">|<\/div><div class="navfooter">|' \
		<${OUTPUT_DIR}/bk01-toc-temp.html >${OUTPUT_DIR}/bk01-toc.html
rm -f ${OUTPUT_DIR}/bk01-toc-temp.html
endef

all: html-all pspdf-a4 pspdf-us

all-a4: html-all pspdf-a4

all-us: html-all pspdf-us

init-html:
	if test "${HAVE_XSL_DIR}" = "0" ; then \
	mv ${XSL_HTML_CHUNK} ${XSL_HTML_CHUNK}-orig; \
	sed -e 's|http\:\/\/docbook.sourceforge.net\/release\/xsl\/current|..\/docbook-xsl|g' < ${XSL_HTML_CHUNK}-orig > ${XSL_HTML_CHUNK}; \
	fi

init-big-html:
	if test "${HAVE_XSL_DIR}" = "0" ; then \
	mv ${XSL_HTML} ${XSL_HTML}-orig; \
	sed -e 's|http\:\/\/docbook.sourceforge.net\/release\/xsl\/current|..\/docbook-xsl|g' < ${XSL_HTML}-orig > ${XSL_HTML}; \
	fi

init-fo-a4:
	if test "${HAVE_XSL_DIR}" = "0" ; then \
	mv ${BOOK_LAYER_A4} ${BOOK_LAYER_A4}-orig; \
	sed -e 's|http\:\/\/docbook.sourceforge.net\/release\/xsl\/current|..\/docbook-xsl|g' < ${BOOK_LAYER_A4}-orig > ${BOOK_LAYER_A4}; \
	fi

init-fo-us:
	if test "${HAVE_XSL_DIR}" = "0" ; then \
	mv ${BOOK_LAYER_US} ${BOOK_LAYER_US}-orig; \
	sed -e 's|http\:\/\/docbook.sourceforge.net\/release\/xsl\/current|..\/docbook-xsl|g' < ${BOOK_LAYER_US}-orig > ${BOOK_LAYER_US}; \
	fi

html: clean-html  validate-all builtdoc-create init-html
	${XSLTPROC} --nonet --output ${BOOK_HTML_XSL} ${BOOK_TEMPLATE} ${BOOK_HTML_XML}
	${XSLTPROC} --nonet --output ${OUTPUT_DIR}/ ${XSL_HTML_CHUNK} ${BOOK_XML_SOURCE}	
	${move-pictures}
	${reinit-html}
	${hackhtmltoc}

html-one-big: clean-html validate-all builtdoc-create init-big-html
	${XSLTPROC} --nonet --output ${BOOK_HTML_XSL} ${BOOK_TEMPLATE} ${BOOK_HTML_XML}
	${XSLTPROC} --nonet --output ${BOOK_HTML_TARGET} ${XSL_HTML} ${BOOK_XML_SOURCE}
	${move-pictures}
	${reinit-big-html}

html-all: clean-html validate-all builtdoc-create init-big-html init-html
	${XSLTPROC} --nonet --output ${BOOK_HTML_XSL} ${BOOK_TEMPLATE} ${BOOK_HTML_XML}
	${XSLTPROC} --nonet --output ${OUTPUT_DIR}/ ${XSL_HTML_CHUNK} ${BOOK_XML_SOURCE}	
	${XSLTPROC} --nonet --output ${BOOK_HTML_TARGET} ${XSL_HTML} ${BOOK_XML_SOURCE}
	${move-pictures}
	${reinit-html}
	${reinit-big-html}
	${hackhtmltoc}

	
pdf-a4: clean-pdf-a4 validate-all fo-a4 builtdoc-create
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -pdf ${BOOK_PDF_A4_TARGET}
	rm -f ${BOOK_FO_TARGET}

pdf-us: clean-pdf-us validate-all fo-us builtdoc-create
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -pdf ${BOOK_PDF_US_TARGET}
	rm -f ${BOOK_FO_TARGET}

ps-a4: clean-ps-a4 validate-all fo-a4 builtdoc-create
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -ps ${BOOK_PS_A4_TARGET}
	rm -f ${BOOK_FO_TARGET}

ps-us: clean-ps-us validate-all fo-us builtdoc-create
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -ps ${BOOK_PS_US_TARGET}
	rm -f ${BOOK_FO_TARGET}

pspdf-a4: clean-pspdf-a4 validate-all fo-a4 builtdoc-create
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -pdf ${BOOK_PDF_A4_TARGET}
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -ps ${BOOK_PS_A4_TARGET}
	rm -f ${BOOK_FO_TARGET}

pspdf-us: clean-pspdf-us validate-all fo-us builtdoc-create
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -pdf ${BOOK_PDF_US_TARGET}
	${RUN_FOP} ${BOOK_TOP} -fo ${BOOK_FO_TARGET} -ps ${BOOK_PS_US_TARGET}
	rm -f ${BOOK_FO_TARGET}

fo-a4:  init-fo-a4
	${XSLTPROC} --nonet --output ${BOOK_XSL_A4} ${BOOK_TEMPLATE} ${BOOK_XML_A4}
	${XSLTPROC} --nonet --output ${BOOK_FO_TARGET} ${BOOK_LAYER_A4} ${BOOK_XML_SOURCE}
	${reinit-fo-a4}

fo-us: init-fo-us
	${XSLTPROC} --nonet --output ${BOOK_XSL_US} ${BOOK_TEMPLATE} ${BOOK_XML_US}
	${XSLTPROC} --nonet --output ${BOOK_FO_TARGET} ${BOOK_LAYER_US} ${BOOK_XML_SOURCE}
	${reinit-fo-us}

validate-all:
	xmllint --noout --nonet --valid ${BOOK_XML_SOURCE}

builtdoc-create:
	mkdir -p ${OUTPUT_DIR}

builtdoc-remove:
	rm -rf ${OUTPUT_DIR}
	
tarballs-remove:
	rm -rf ${TARBALL_DIR}

clean-html: clean-xsl-html
	rm -f ${OUTPUT_DIR}/*.html
	rm -f ${OUTPUT_DIR}/*.css
	rm -rf ${OUTPUT_FIG}
	rm -rf ${OUTPUT_IMG}

clean-fo:
	rm -f ${BOOK_FO_TARGET}

clean-xsl-a4:
		rm -f ${BOOK_XSL_A4}

clean-xsl-us:
	rm -f ${BOOK_XSL_US}

clean-xsl-html:
	rm -f ${BOOK_HTML_XSL}

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

clean-all: clean-html clean-pspdf-a4 clean-pspdf-us  builtdoc-remove tarballs-remove

clean: clean-all

distclean: clean

tarball-all: tarballs-remove all
	mkdir -p ${TARBALL_DIR}/${BOOK_PREFIX}-html-chunked-${VERSION}/imgs
	mkdir -p ${TARBALL_DIR}/${BOOK_PREFIX}-html-chunked-${VERSION}/figures
	mkdir -p ${TARBALL_DIR}/${BOOK_PREFIX}-html-bigfile-${VERSION}/imgs
	mkdir -p ${TARBALL_DIR}/${BOOK_PREFIX}-html-bigfile-${VERSION}/figures
	cp ${OUTPUT_DIR}/*.p* ${TARBALL_DIR}
	cp ${OUTPUT_DIR}/book.html ${TARBALL_DIR}/${BOOK_PREFIX}-html-bigfile-${VERSION}
	cp ${OUTPUT_DIR}/manual.css ${TARBALL_DIR}/${BOOK_PREFIX}-html-bigfile-${VERSION}
	cp ${OUTPUT_DIR}/imgs/*.png ${TARBALL_DIR}/${BOOK_PREFIX}-html-bigfile-${VERSION}/imgs
	cp ${OUTPUT_DIR}/figures/*.png ${TARBALL_DIR}/${BOOK_PREFIX}-html-bigfile-${VERSION}/figures
	cp ${OUTPUT_DIR}/*.html ${TARBALL_DIR}/${BOOK_PREFIX}-html-chunked-${VERSION}
	cp ${OUTPUT_DIR}/manual.css ${TARBALL_DIR}/${BOOK_PREFIX}-html-chunked-${VERSION}
	cp ${OUTPUT_DIR}/imgs/*.png ${TARBALL_DIR}/${BOOK_PREFIX}-html-chunked-${VERSION}/imgs
	cp ${OUTPUT_DIR}/figures/*.png ${TARBALL_DIR}/${BOOK_PREFIX}-html-chunked-${VERSION}/figures
	cd ${TARBALL_DIR}; \
	mv book-a4.pdf ${BOOK_PREFIX}-pdf-a4-${VERSION}.pdf; \
	tar -jvcf ${BOOK_PREFIX}-pdf-a4-${VERSION}.tar.bz2 ${BOOK_PREFIX}-pdf-a4-${VERSION}.pdf; 
	cd ${TARBALL_DIR}; \
	mv book-us.pdf ${BOOK_PREFIX}-pdf-us-${VERSION}.pdf; \
	tar -jvcf ${BOOK_PREFIX}-pdf-us-${VERSION}.tar.bz2 ${BOOK_PREFIX}-pdf-us-${VERSION}.pdf;
	cd ${TARBALL_DIR}; \
	mv book-a4.ps ${BOOK_PREFIX}-ps-a4-${VERSION}.ps; \
	tar -jvcf ${BOOK_PREFIX}-ps-a4-${VERSION}.tar.bz2 ${BOOK_PREFIX}-ps-a4-${VERSION}.ps;
	cd ${TARBALL_DIR}; \
	mv book-us.ps ${BOOK_PREFIX}-ps-us-${VERSION}.ps; \
	tar -jvcf ${BOOK_PREFIX}-ps-us-${VERSION}.tar.bz2 ${BOOK_PREFIX}-ps-us-${VERSION}.ps;
	cd ${TARBALL_DIR}; \
	tar -jvcf ${BOOK_PREFIX}-html-bigfile-${VERSION}.tar.bz2 ${BOOK_PREFIX}-html-bigfile-${VERSION}/; 
	cd ${TARBALL_DIR}; \
	rm -f ${BOOK_PREFIX}-html-chunked-${VERSION}/book.html; \
	tar -jvcf ${BOOK_PREFIX}-html-chunked-${VERSION}.tar.bz2 ${BOOK_PREFIX}-html-chunked-${VERSION}/;
	rm -f ${TARBALL_DIR}/*.pdf
	rm -f ${TARBALL_DIR}/*.ps
	rm -Rf ${TARBALL_DIR}/${BOOK_PREFIX}-html-bigfile-${VERSION}
	rm -Rf ${TARBALL_DIR}/${BOOK_PREFIX}-html-chunked-${VERSION}
