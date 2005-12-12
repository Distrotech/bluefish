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
<!-- Chapter autolabelling -->
<xsl:param name="chapter.autolabel" select="'I'"/>
<!-- Section autolabelling -->
<xsl:variable name="section.autolabel" select="'1'"/>
<!-- Css stylesheet -->
<xsl:param name='html.stylesheet' select="'manual.css'"/>
<!-- Use admon graphics -->
<xsl:param name="admon.graphics" select="'1'"/>
<!-- Just use graphics for admon graphics -->
<xsl:param name="admon.textlabel" select="'0'"/>
<!-- Customized path for admon graphics -->
<xsl:param name="admon.graphics.path" select="'imgs/'"/>
<!-- Remove indentation to admon graphics -->
<xsl:param name="admon.style">
  <xsl:text>margin-left: 0in; margin-right: 0in;</xsl:text>
</xsl:param>
</xsl:stylesheet>
