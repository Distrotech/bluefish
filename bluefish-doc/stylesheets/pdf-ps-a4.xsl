<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
	xmlns:fo="http://www.w3.org/1999/XSL/Format">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl"/>
<!-- <xsl:import href="../xsl/fo/docbook.xsl"/> -->
<xsl:import href="titlepage-a4.xsl"/>

<!--
this file belongs to bluefish, a web development environment
Copyright (C) 2005-2006 The Bluefish Project Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
-->

<!-- For bookmarks -->
<xsl:param name="fop.extensions" select="1"></xsl:param>

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

<!-- No headers on blank pages -->
<xsl:param name="headers.on.blank.pages" select="0"></xsl:param>

<!-- Indentation of the titles for sections -->
<xsl:param name="title.margin.left" select="'-1pc'"/>

<!-- Depth of table of contents -->
<xsl:param name="toc.section.depth">3</xsl:param>

<!-- Insert list of procedures in toc -->
<xsl:param name="generate.toc">
book      toc,title,figure,example,procedure,table
</xsl:param>

<!-- Chapter autolabelling -->
<xsl:param name="chapter.autolabel" select="'I'"/>

<!-- Section autolabelling -->
<xsl:param name="section.autolabel" select="'1'"/>

<!-- Format variable lists as blocks -->
<xsl:param name="variablelist.as.blocks" select="1"></xsl:param>

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

<!-- Customization of chapter, appendix, preface title
Made here as it really overrides the setting in titlepage -->
<xsl:attribute-set name="component.title.properties">
  <xsl:attribute name="keep-with-next.within-column">always</xsl:attribute>
  <xsl:attribute name="text-align">
    <xsl:choose>
      <xsl:when test="((ancestor-or-self::preface[1]) or (ancestor-or-self::chapter[1]) or (ancestor-or-self::appendix[1]))">center</xsl:when>
      <xsl:otherwise>left</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
  <xsl:attribute name="start-indent"><xsl:value-of select="$title.margin.left"/></xsl:attribute>
</xsl:attribute-set>
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
  <xsl:attribute name="font-style">italic</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.6em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level6.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master - 1"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
  <xsl:attribute name="font-weight">normal</xsl:attribute>
  <xsl:attribute name="font-style">italic</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.6em</xsl:attribute>
</xsl:attribute-set>

<!-- Normal paragraph spacing -->
<xsl:attribute-set name="normal.para.spacing">
   <xsl:attribute name="space-before.optimum">0.5em</xsl:attribute>
   <xsl:attribute name="space-before.minimum">0.5em</xsl:attribute>
   <xsl:attribute name="space-before.maximum">0.5em</xsl:attribute>
	<xsl:attribute name="text-indent">0em</xsl:attribute>
</xsl:attribute-set>

<!-- Spacing before and after lists -->
<xsl:attribute-set name="list.block.spacing">
  <xsl:attribute name="space-before.optimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.4em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.4em</xsl:attribute>
</xsl:attribute-set>

<!-- Spacing between list items -->
<xsl:attribute-set name="list.item.spacing">
  <xsl:attribute name="space-before.optimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.3em</xsl:attribute>
</xsl:attribute-set>

<!-- Spacing between compact list items -->
<xsl:attribute-set name="compact.list.item.spacing">
  <xsl:attribute name="space-before.optimum">0em</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0em</xsl:attribute>
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

<!-- Prohibites hyphenation in text -->
<xsl:param name="hyphenate">false</xsl:param>

<!-- Allow hyphenation for urls -->
<xsl:param name="ulink.hyphenate" select="'&#x200B;'"/>

<!-- Defined allowed characters for ulink hyphenation -->
<xsl:param name="ulink.hyphenate.chars">:/@&amp;?.=#</xsl:param>

<!-- Insert page number for ulink -->
<xsl:param name="insert.link.page.number">yes</xsl:param>

<!-- For figures, procedure, example, table -->
<xsl:attribute-set name="formal.object.properties">
  <xsl:attribute name="space-before.minimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.4em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.3em</xsl:attribute>
  <xsl:attribute name="keep-together.within-column">always</xsl:attribute>
</xsl:attribute-set>

<!-- Style and placement of title for procedure, example, table, figures
 And space after procedure title -->
<xsl:attribute-set name="formal.title.properties">
  <xsl:attribute name="font-weight">bold</xsl:attribute>
  <xsl:attribute name="font-size">10pt</xsl:attribute>
  <xsl:attribute name="hyphenate">false</xsl:attribute>
  <xsl:attribute name="space-after.minimum">
    <xsl:choose>
      <xsl:when test="self::procedure">0em</xsl:when>
      <xsl:otherwise>inherit</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
  <xsl:attribute name="space-after.optimum">
    <xsl:choose>
      <xsl:when test="self::procedure">0em</xsl:when>
      <xsl:otherwise>inherit</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
  <xsl:attribute name="space-after.maximum">
    <xsl:choose>
      <xsl:when test="self::procedure">0em</xsl:when>
      <xsl:otherwise>inherit</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
  <xsl:attribute name="text-align">
    <xsl:choose>
      <xsl:when test="self::table">center</xsl:when>
      <xsl:otherwise>left</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
</xsl:attribute-set>

<!-- Links color -->
<xsl:attribute-set name="xref.properties">
      <xsl:attribute name="color">blue</xsl:attribute>
</xsl:attribute-set>

<xsl:param name="insert.xref.page.number">yes</xsl:param>

<!-- Guimenu color -->
<xsl:template match="guimenu/|guimenuitem/|guilabel/|guisubmenu/">
<fo:wrapper color="#806B38">
    <xsl:apply-imports/>
  </fo:wrapper>
</xsl:template>

<!-- User input and screen -->
<xsl:template match="userinput/|screen/">
<fo:wrapper color="#38805b">
    <xsl:apply-imports/>
  </fo:wrapper>
</xsl:template>

<!-- Bold face for prompt -->
<xsl:template match="screen/prompt/"> 
<fo:wrapper font-weight="bold">
    <xsl:apply-imports/>
  </fo:wrapper>
</xsl:template> 

<!--Term color -->
<xsl:template match="variablelist/varlistentry/term/">
<fo:wrapper color="#5e3880">
    <xsl:apply-imports/>
  </fo:wrapper>
</xsl:template>

<!-- First bookmark points to second page of cover -->
<xsl:template name="book.titlepage.separator"/>

<!-- Use admon graphics -->
<xsl:param name="admon.graphics" select="1"/>

<!-- Customized path for admon.graphics -->
<xsl:param name="admon.graphics.path" select="'imgs/'"/>

<!-- Just use graphics for admon graphics -->
<xsl:param name="admon.textlabel" select="'0'"/>

<!-- Style of admonitions
     Ugly hack on space-after-minimum to at least get
     a minimal space when admon contents are only one line -->
<xsl:attribute-set name="graphical.admonition.properties">
  <xsl:attribute name="space-before.minimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.5em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.5em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.8em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.8em</xsl:attribute>
</xsl:attribute-set>

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

<!-- For shortcuts table -->
<xsl:attribute-set name="table.cell.padding">
    <xsl:attribute name="padding-left">2px</xsl:attribute>
    <xsl:attribute name="padding-right">1px</xsl:attribute>
    <xsl:attribute name="padding-top">4px</xsl:attribute>
    <xsl:attribute name="padding-bottom">4px</xsl:attribute>
 </xsl:attribute-set>
 
<!-- Put each term of a multiple terms variablelistentry on its own line -->
<xsl:param name="variablelist.term.break.after" select="'1'"/>
<!-- Remove the separator between terms -->
<xsl:param name="variablelist.term.separator"></xsl:param>

<!-- Chapters, Preface, Appendixes in toc in bold -->
<xsl:attribute-set name="toc.line.properties">
  <xsl:attribute name="font-weight">
  	<xsl:choose>
    	<xsl:when test="self::chapter | self::preface | self::appendix">bold</xsl:when>
    	<xsl:otherwise>normal</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
  <xsl:attribute name="font-style">
  	<xsl:choose>
    	<xsl:when test="self::sect3">italic</xsl:when>
    	<xsl:otherwise>normal</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
</xsl:attribute-set>

</xsl:stylesheet>  
