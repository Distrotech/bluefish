<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns="http://www.w3.org/TR/xhtml1/transitional"
                exclude-result-prefixes="#default">
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/chunk.xsl"/>
<!-- <xsl:import href="../xsl/html/chunk.xsl"/> -->
<!-- Work around a bug in stylesheets to get table of contents outside of titlepage -->
<xsl:include href="chunk-common-patched.xsl"/>
<xsl:include href="titlepage-html.xsl"/>
<!-- Define the output encoding as UTF-8 -->
<xsl:param name="chunker.output.encoding" select="'UTF-8'"/>
 <!-- Insert list of procedures in toc -->
<xsl:param name="generate.toc">
book	toc,title,figure,example,procedure
chapter		toc
preface		toc
appendix	toc
sect1			toc
sect2			toc
sect3			toc
section		toc
</xsl:param>
<!-- Toc depth -->
<xsl:variable name="toc.section.depth">3</xsl:variable>
<!-- Allow toc in section -->
<xsl:param name="generate.section.toc.level" select="2"/>
<!-- Section autolabelling -->
<xsl:variable name="section.autolabel">f</xsl:variable>
<!-- Css stylesheet -->
<xsl:param name="html.stylesheet" select="'manual.css'"/>
<!-- Separate toc -->
<xsl:param name="chunk.tocs.and.lots" select="'1'"/>
</xsl:stylesheet>
