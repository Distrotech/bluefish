<?xml version="1.0" encoding="UTF-8"?><xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
	xmlns:fo="http://www.w3.org/1999/XSL/Format">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl"/>
<!-- <xsl:import href="../xsl/fo/docbook.xsl"/> -->
<xsl:import href="titlepage-a4.xsl"/>

<!-- Allow to use extensions -->
<xsl:param name="use.extensions" select="'1'"></xsl:param>
<!-- For bookmarks -->
<xsl:param name="fop.extensions" select="1"></xsl:param>
<!-- For revision history -->
<xsl:param name="tablecolumns.extension" select="'1'"></xsl:param>

<!-- Ensure the page orientation is portrait -->
<xsl:param name="page.orientation" select="portrait"></xsl:param>

<!-- A4 format -->
<xsl:param name="paper.type" select="'A4'"></xsl:param>
<xsl:param name="page.width" select="'210mm'"></xsl:param>
<xsl:param name="page.height" select="'297mm'"></xsl:param>

<!-- Double-sided -->
<xsl:param name="double.sided" select="1"></xsl:param>

<!-- Margins -->
<xsl:param name="page.margin.top" select="'7.5mm'"></xsl:param>
<xsl:param name="page.margin.bottom" select="'7.5mm'"></xsl:param>
<xsl:param name="region.before.extent" select="'5mm'"></xsl:param>
<xsl:param name="region.after.extent" select="'5mm'"></xsl:param>
<xsl:param name="body.margin.top" select="'10mm'"></xsl:param>
<xsl:param name="body.margin.bottom" select="'10mm'"></xsl:param>
<xsl:param name="page.margin.inner" select="'15mm'"></xsl:param>
<xsl:param name="page.margin.outer" select="'12mm'"></xsl:param>

<!-- Insert list of procedures in toc -->
<xsl:param name="generate.toc">
book      toc,title,figure,example,procedure
</xsl:param>

<!-- Depth of table of contents -->
<xsl:param name="toc.section.depth">3</xsl:param>

<!-- No headers on blank pages -->
<xsl:param name="headers.on.blank.pages" select="0"></xsl:param>

<!-- Negative indentation of the titles for sections-->
<xsl:param name="title.margin.left" select="'-1pc'"></xsl:param>

<!-- Format variable lists as blocks -->
<xsl:param name="variablelist.as.blocks" select="1"></xsl:param>

<!-- Don't show the url after the text of the link -->
<xsl:param name="ulink.show" select="0"></xsl:param>

<!-- Size of the headers (a table with three columns) -->
<xsl:param name="header.column.widths" select="'1 8 1'"></xsl:param>

<!-- Length of footnotes ruler -->
<xsl:template name="footnote-separator">
  <fo:static-content flow-name="xsl-footnote-separator">
    <fo:block>
      <fo:leader color="black" leader-pattern="rule" leader-length="10mm"/>
    </fo:block>
  </fo:static-content>
</xsl:template>

<!-- Size of footnotes -->
<xsl:param name="footnote.font.size" select="'8pt'"></xsl:param>

<!-- Size of body font -->
<xsl:param name="body.font.master">10</xsl:param>
<xsl:param name="body.font.size">
 <xsl:value-of select="$body.font.master"/><xsl:text>pt</xsl:text>
</xsl:param>

<!-- Spaces before title -->
<xsl:attribute-set name="section.title.properties">
  <xsl:attribute name="font-family">
    <xsl:value-of select="$title.font.family"/>
  </xsl:attribute>
  <xsl:attribute name="font-weight">bold</xsl:attribute>
  <!-- font size is calculated dynamically by section.heading template -->
  <xsl:attribute name="space-before.minimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.8em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.8em</xsl:attribute>
  <xsl:attribute name="keep-with-next.within-page">always</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level1.properties">
  <xsl:attribute name="font-size">16pt</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.8em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.8em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.8em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level2.properties">
  <xsl:attribute name="font-size">13pt</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.7em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.7em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.7em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level3.properties">
  <xsl:attribute name="font-size">10pt</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.6em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level4.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master -1"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.6em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level5.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master - 1"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
  <xsl:attribute name="font-style">italic</xsl:attribute>  <xsl:attribute name="space-before.minimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.6em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level6.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master - 1"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
  <xsl:attribute name="font-weight">normal</xsl:attribute>  <xsl:attribute name="font-style">italic</xsl:attribute>  <xsl:attribute name="space-before.minimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.6em</xsl:attribute>
</xsl:attribute-set>

<!-- Normal paragraph spacing -->
<xsl:attribute-set name="normal.para.spacing">   <xsl:attribute name="space-before.optimum">0.5em</xsl:attribute>   <xsl:attribute name="space-before.minimum">0.5em</xsl:attribute>   <xsl:attribute name="space-before.maximum">0.5em</xsl:attribute>	<xsl:attribute name="text-indent">0em</xsl:attribute>
</xsl:attribute-set>

<!-- Spacing before and after lists -->
<xsl:attribute-set name="list.block.spacing">
  <xsl:attribute name="space-before.optimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.3em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.3em</xsl:attribute>
</xsl:attribute-set>

<!-- Spacing between list items -->
<xsl:attribute-set name="list.item.spacing">
  <xsl:attribute name="space-before.optimum">0.2em</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.2em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.2em</xsl:attribute>
</xsl:attribute-set>

<!-- Margins for toc -->
<xsl:attribute-set name="toc.margin.properties">
  <xsl:attribute name="space-before.minimum">0.5em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.5em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.5em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.5em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.5em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.5em</xsl:attribute>
</xsl:attribute-set>

<!-- Title placement -->
<xsl:param name="formal.title.placement">
figure after
example before
equation before
table before
procedure before
</xsl:param>

<!-- Prohibites hyphenation -->
<xsl:param name="hyphenate">false</xsl:param>

<!-- For figures -->
<xsl:attribute-set name="formal.object.properties">
  <xsl:attribute name="space-before.minimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.4em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.3em</xsl:attribute>
  <xsl:attribute name="keep-together.within-column">always</xsl:attribute>
</xsl:attribute-set>

<!-- Links color -->
<xsl:attribute-set name="xref.properties">      <xsl:attribute name="color">blue</xsl:attribute></xsl:attribute-set>

<!-- Note, tip blue color -->
<xsl:template match="varlistentry/term/emphasis/">
<fo:wrapper color="#3366cc">
    <xsl:apply-imports/>
  </fo:wrapper>
</xsl:template>

<!-- First bookmark points to second page of cover -->
<xsl:template name="book.titlepage.separator">
</xsl:template>

<!-- For page breaks, don't forget to add hardcoded ones when book is finished -->
<xsl:template match="processing-instruction('pagebreak')">
        <fo:block break-after="page"/>
</xsl:template>
<xsl:template match="processing-instruction('pagebreaka4')">
        <fo:block break-after="page"/>
</xsl:template>

<!-- For line breaks, don't forget to add hardcoded ones when book is finished -->
<xsl:template match="processing-instruction('linebreak')">
        <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreaka4')">
        <fo:block>&#160;</fo:block>
</xsl:template>

</xsl:stylesheet>  
