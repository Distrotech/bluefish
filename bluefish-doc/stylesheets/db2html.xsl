<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns="http://www.w3.org/TR/xhtml1/transitional"
                exclude-result-prefixes="#default">
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/chunk.xsl"/>
<!-- <xsl:import href="../xsl/html/chunk.xsl"/> -->
<xsl:import href="titlepage-html.xsl"/>
<!-- Allow to use extensions -->
<xsl:param name="use.extensions" select="'1'"></xsl:param>
<!-- For revision history -->
<xsl:param name="tablecolumns.extension" select="'1'"></xsl:param>
<!-- Define the output encoding as UTF-8 -->
<xsl:param name="chunker.output.encoding" select="'UTF-8'"/>
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
