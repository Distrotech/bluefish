<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
	xmlns:fo="http://www.w3.org/1999/XSL/Format">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl"/>
<!-- <xsl:import href="../xsl/fo/docbook.xsl"/> -->
<xsl:import href="titlepage-a4.xsl"/>

<!--
this file belongs to bluefish, a web development environment
Copyright (C) 2005-2007 The Bluefish Project Team

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

<!-- Define the output encoding as UTF-8 -->
<xsl:output method="xml"
            encoding="UTF-8"
            indent="yes"/>

<!-- For bookmarks -->
<xsl:param name="fop.extensions" select="1"></xsl:param>

<!-- Get rid of draft mode -->
<!-- Actually it does not output error when using fop.sh, but does it when using fop, so that it is needed -->
<xsl:param name="draft.mode" select="'no'"></xsl:param>

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

<!-- No header rule on title page -->
 <xsl:template name="head.sep.rule">
   <xsl:param name="pageclass"/>
   <xsl:param name="sequence"/>
   <xsl:param name="gentext-key"/>
   <xsl:if test="$header.rule != 0 and $pageclass != 'titlepage' and $sequence != 'first'">
     <xsl:attribute name="border-bottom-width">0.5pt</xsl:attribute>
     <xsl:attribute name="border-bottom-style">solid</xsl:attribute>
     <xsl:attribute name="border-bottom-color">black</xsl:attribute>
   </xsl:if>
 </xsl:template>

<!-- No footer rule on book title page -->
 <xsl:template name="foot.sep.rule">
   <xsl:param name="pageclass"/>
   <xsl:param name="sequence"/>
   <xsl:param name="gentext-key"/>
   <xsl:if test="$footer.rule != 0 and $pageclass != 'titlepage'">
     <xsl:attribute name="border-top-width">0.5pt</xsl:attribute>
     <xsl:attribute name="border-top-style">solid</xsl:attribute>
     <xsl:attribute name="border-top-color">black</xsl:attribute>
   </xsl:if>
 </xsl:template>

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
  <xsl:attribute name="space-before.minimum">0.75em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.8em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.85em</xsl:attribute>
  <xsl:attribute name="keep-with-next.within-page">always</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level1.properties">
  <xsl:attribute name="font-size">16pt</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.75em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.8em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.85em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level2.properties">
  <xsl:attribute name="font-size">13pt</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.65em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.7em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.75em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level3.properties">
  <xsl:attribute name="font-size">10pt</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.55em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.65em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level4.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master -1"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.55em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.65em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level5.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master - 1"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
  <xsl:attribute name="font-style">italic</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.55em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.65em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="section.title.level6.properties">
  <xsl:attribute name="font-size">
    <xsl:value-of select="$body.font.master - 1"/>
    <xsl:text>pt</xsl:text>
  </xsl:attribute>
  <xsl:attribute name="font-weight">normal</xsl:attribute>
  <xsl:attribute name="font-style">italic</xsl:attribute>
  <xsl:attribute name="space-before.minimum">0.55em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.65em</xsl:attribute>
</xsl:attribute-set>

<!-- Normal paragraph spacing -->
<xsl:attribute-set name="normal.para.spacing">
   <xsl:attribute name="space-before.minimum">0.45em</xsl:attribute>
   <xsl:attribute name="space-before.optimum">0.5em</xsl:attribute>
   <xsl:attribute name="space-before.maximum">0.55em</xsl:attribute>
	<xsl:attribute name="text-indent">0em</xsl:attribute>
</xsl:attribute-set>

<!-- Spacing before and after lists -->
<xsl:attribute-set name="list.block.spacing">
  <xsl:attribute name="space-before.minimum">0.35em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.5em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.45em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.35em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.45em</xsl:attribute>
</xsl:attribute-set>

<!-- Spacing between list items -->
<xsl:attribute-set name="list.item.spacing">
   <xsl:attribute name="space-before.minimum">0.25em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.3em</xsl:attribute>
 <xsl:attribute name="space-before.maximum">0.35em</xsl:attribute>
</xsl:attribute-set>

<!-- Spacing between compact list items -->
<xsl:attribute-set name="compact.list.item.spacing">
  <xsl:attribute name="space-before.minimum">0em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.1em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.15em</xsl:attribute>
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
  <xsl:attribute name="space-before.minimum">0.35em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.4em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.45em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.25em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.3em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.35em</xsl:attribute>
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
	<xsl:attribute name="color">
			<xsl:choose>
			<xsl:when test="(ancestor-or-self::glossterm[1])">red</xsl:when>
			<xsl:otherwise>blue</xsl:otherwise>
			</xsl:choose>
	</xsl:attribute>
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
     To get a minimal space when admon contents are only one line -->
<xsl:attribute-set name="graphical.admonition.properties">
  <xsl:attribute name="space-before.minimum">0.45em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.5em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.55em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.7em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.75em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.8em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="admonition.properties">
  <xsl:attribute name="space-before.minimum">0.15em</xsl:attribute>
  <xsl:attribute name="space-before.optimum">0.2em</xsl:attribute>
  <xsl:attribute name="space-before.maximum">0.25em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.15em</xsl:attribute>
  <xsl:attribute name="space-after.optimum">0.2em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">0.25em</xsl:attribute>
</xsl:attribute-set>

<!-- To get less space between graphic and text in admonition -->
<xsl:template name="graphical.admonition">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="graphic.width">
     <xsl:apply-templates select="." mode="admon.graphic.width"/>
  </xsl:variable>

  <fo:block id="{$id}"
            xsl:use-attribute-sets="graphical.admonition.properties">
    <fo:list-block provisional-distance-between-starts="{$graphic.width}"
                    provisional-label-separation="0pt">
      <fo:list-item>
          <fo:list-item-label end-indent="label-end()">
            <fo:block>
              <fo:external-graphic width="auto" height="auto"
                                         content-width="{$graphic.width}" >
                <xsl:attribute name="src">
                  <xsl:call-template name="admon.graphic"/>
                </xsl:attribute>
              </fo:external-graphic>
            </fo:block>
          </fo:list-item-label>
          <fo:list-item-body start-indent="body-start()">
            <xsl:if test="$admon.textlabel != 0 or title">
              <fo:block xsl:use-attribute-sets="admonition.title.properties">
                <xsl:apply-templates select="." mode="object.title.markup"/>
              </fo:block>
            </xsl:if>
            <fo:block xsl:use-attribute-sets="admonition.properties">
              <xsl:apply-templates/>
            </fo:block>
          </fo:list-item-body>
      </fo:list-item>
    </fo:list-block>
  </fo:block>
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
<xsl:template match="processing-instruction('linebreak2')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreaka42')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreak3')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
 </xsl:template>
<xsl:template match="processing-instruction('linebreaka43')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreak4')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
 </xsl:template>
<xsl:template match="processing-instruction('linebreaka44')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreak5')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreaka45')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreak6')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreaka46')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreak7')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreaka47')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreak8')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreaka48')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreak9')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreaka49')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
     <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreak10')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>
<xsl:template match="processing-instruction('linebreaka410')">
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
      <fo:block>&#160;</fo:block>
</xsl:template>

<!-- For shortcuts table -->
<xsl:attribute-set name="table.cell.padding">
    <xsl:attribute name="padding-left">2px</xsl:attribute>
    <xsl:attribute name="padding-right">1px</xsl:attribute>
    <xsl:attribute name="padding-top">4px</xsl:attribute>
    <xsl:attribute name="padding-bottom">4px</xsl:attribute>
 </xsl:attribute-set>
 <xsl:param name="table.frame.border.color">#96a5b8</xsl:param>
<xsl:param name="table.cell.border.color">#96a5b8</xsl:param>

<!-- Put each term of a multiple terms variablelistentry on its own line -->
<xsl:param name="variablelist.term.break.after" select="'1'"/>
<!-- Remove the separator between terms -->
<xsl:param name="variablelist.term.separator"></xsl:param>

<!-- Chapters, Preface, Appendixes in toc in bold -->
<xsl:attribute-set name="toc.line.properties">
  <xsl:attribute name="font-weight">
  	<xsl:choose>
    	<xsl:when test="self::part | self::chapter | self::preface | self::appendix">bold</xsl:when>
    	<xsl:otherwise>normal</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
  <xsl:attribute name="font-style">
  	<xsl:choose>
    	<xsl:when test="self::sect3">italic</xsl:when>
    	<xsl:otherwise>normal</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
    <xsl:attribute name="space-before.minimum">
 	<xsl:choose>
    	<xsl:when test="self::part">0.7em</xsl:when>
    	<xsl:when test="self::chapter | self::preface | self::appendix">0.45em</xsl:when>
    	<xsl:otherwise>inherit</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
    <xsl:attribute name="space-before.maximum">
 	<xsl:choose>
    	<xsl:when test="self::part">0.75em</xsl:when>
    	<xsl:when test="self::chapter | self::preface | self::appendix">0.55em</xsl:when>
    	<xsl:otherwise>inherit</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
    <xsl:attribute name="space-before.optimum">
 	<xsl:choose>
    	<xsl:when test="self::part">0.8em</xsl:when>
    	<xsl:when test="self::chapter | self::preface | self::appendix">0.5em</xsl:when>
    	<xsl:otherwise>inherit</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
</xsl:attribute-set>

<!-- Revision history appearance -->
<xsl:attribute-set name="revhistory.title.properties">
	<xsl:attribute name="font-weight">bold</xsl:attribute>
 	<xsl:attribute name="text-align">center</xsl:attribute>
 	<xsl:attribute name="space-after.minimum">0.75em</xsl:attribute>
  	<xsl:attribute name="space-after.optimum">0.8em</xsl:attribute>
  	<xsl:attribute name="space-after.maximum">0.85em</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="revhistory.table.properties">
	<xsl:attribute name="padding">
	<xsl:choose>
	<xsl:when test="ancestor::appendix">0.25em</xsl:when>
	<xsl:otherwise>0em</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
	<xsl:attribute name="background-color">white</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="revhistory.table.cell.properties">
	<xsl:attribute name="background-color">
	<xsl:choose>
	<xsl:when test="revnumber | date">#eee</xsl:when>
	<xsl:otherwise>white</xsl:otherwise>
    </xsl:choose>
  </xsl:attribute>
</xsl:attribute-set>

<xsl:template match="revision/revremark">
	<fo:block text-indent="1em" background-color="white">
	<xsl:apply-templates/>
	</fo:block>
</xsl:template>

<xsl:template match="revision/revdescription">
	<fo:block background-color="white">
	<xsl:apply-templates/>
	</fo:block>
</xsl:template>

<!-- Part autolabelling -->
<xsl:param name="part.autolabel" select="'1'"/>

<!-- Indent all lists -->
<xsl:attribute-set name="list.block.spacing">
  <xsl:attribute name="margin-left">1em</xsl:attribute>
</xsl:attribute-set>

<!-- Collate copyright years into range -->
<xsl:param name="make.year.ranges" select="1"></xsl:param>        

<!-- Segmented list as tables workaround for fop-->
<xsl:template match="segmentedlist" mode="seglist-table">
 	<xsl:attribute name="space-before.minimum">0.75em</xsl:attribute>
  	<xsl:attribute name="space-before.optimum">0.8em</xsl:attribute>
  	<xsl:attribute name="space-before.maximum">0.85em</xsl:attribute>
  <xsl:apply-templates select="title" mode="list.title.mode"/>
  <fo:table table-layout="fixed" border-collapse="separate"
			border-left="solid" 
			border-top="none" border-bottom="none" 
			border-right="none" border-left-color="#96a5b8" >
    <fo:table-column column-number="1" column-width="96px"/>
    <fo:table-column column-number="2" column-width="170px"/> 
    <fo:table-header>
      <fo:table-row>
        <xsl:apply-templates select="segtitle" mode="seglist-table"/>
      </fo:table-row>
    </fo:table-header>
    <fo:table-body>
      <xsl:apply-templates select="seglistitem" mode="seglist-table"/>
    </fo:table-body>
  </fo:table>
</xsl:template>
<!-- Same colored border as other tables --> 
<xsl:template match="segtitle" mode="seglist-table">
  <fo:table-cell>
    <fo:block font-weight="bold" text-align="center" border-right="solid" 
			border-top="solid" border-bottom="solid" 
			border-left="none" border-top-color="#96a5b8" 
			border-bottom-color="#96a5b8" border-right-color="#96a5b8" 
			padding-top="4px" padding-bottom="4px">
     <xsl:apply-templates/>
    </fo:block>
  </fo:table-cell>
</xsl:template>
<xsl:template match="seg" mode="seglist-table">
  <fo:table-cell>
    <fo:block border-right="solid" border-top="solid" border-bottom="solid" 
			border-left="none" border-top-color="#96a5b8" 
			border-bottom-color="#96a5b8" border-right-color="#96a5b8" 
			text-indent="0.5em" padding-top="4px" padding-bottom="4px">
      <xsl:apply-templates/>
    </fo:block>
  </fo:table-cell>
</xsl:template>

<!-- Only for use with docbook-snapshot after 0.70.1 -->
<!-- Put delimiters around email address -->
<xsl:param name="email.delimiters.enabled">1</xsl:param>

<!-- Sidebar for parts -->
<xsl:attribute-set name="sidebar.properties">
  <xsl:attribute name="border-style">solid</xsl:attribute>
  <xsl:attribute name="border-width">0.25pt</xsl:attribute>
  <xsl:attribute name="border-left-color">black</xsl:attribute>
  <xsl:attribute name="border-top-color">white</xsl:attribute>
  <xsl:attribute name="border-bottom-color">white</xsl:attribute>
  <xsl:attribute name="border-right-color">white</xsl:attribute>
   <xsl:attribute name="background-color">white</xsl:attribute>
  <xsl:attribute name="padding-left">12pt</xsl:attribute>
  <xsl:attribute name="padding-right">12pt</xsl:attribute>
  <xsl:attribute name="padding-top">0pt</xsl:attribute>
  <xsl:attribute name="padding-bottom">0pt</xsl:attribute>
  <xsl:attribute name="margin-left">4cm</xsl:attribute>
  <xsl:attribute name="margin-right">0pt</xsl:attribute> 
  <xsl:attribute name="font-size">14pt</xsl:attribute> 
</xsl:attribute-set>

<xsl:param name="glossterm.auto.link">1</xsl:param>
</xsl:stylesheet>  
