<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
	version='1.0' 
	xmlns="http://www.w3.org/TR/xhtml1/transitional" 
	exclude-result-prefixes="#default">
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl"/>
<!-- <xsl:import href="../xsl/html/docbook.xsl"/> -->
<xsl:include href="titlepage-html.xsl"/>
<!-- Define the output encoding as UTF-8 -->
<xsl:output method="html"
	encoding="UTF-8"
	indent="no"/>
<!-- Insert list of procedures in toc -->
<xsl:param name="generate.toc">
book      toc,title,figure,example,procedure
</xsl:param>
<!-- Toc depth -->
<xsl:variable name="toc.section.depth">3</xsl:variable>
<!-- Section autolabelling -->
<xsl:variable name="section.autolabel">f</xsl:variable>
<!-- Css stylesheet -->
<xsl:param name='html.stylesheet' select="'manual.css'"/>
</xsl:stylesheet>
