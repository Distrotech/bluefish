<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">
<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/html/docbook.xsl"/>
<!-- <xsl:import href="../xsl/html/docbook.xsl"/> -->
<xsl:import href="titlepage-html.xsl"/>

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
<xsl:output method="html"
            encoding="UTF-8"
            indent="yes"
            doctype-public="-//W3C//DTD HTML 4.01 Transitional//EN"
            doctype-system="http://www.w3.org/TR/html4/loose.dtd"/>
<!-- Make sure, the result has indentation -->
<xsl:param name="chunker.output.indent" select="'yes'"/>
<!-- Define the output encoding as UTF-8 -->
<xsl:param name="chunker.output.encoding" select="'UTF-8'"/>
<!-- Insert list of procedures in toc -->
<xsl:param name="generate.toc">
book      toc,title,figure,example,procedure,table
</xsl:param>
<!-- Title placement -->
<xsl:param name="formal.title.placement">
figure after
example before
equation before
table before
procedure before
</xsl:param>
<!-- Toc depth -->
<xsl:variable name="toc.section.depth">3</xsl:variable>
<!-- Part autolabelling -->
<xsl:param name="part.autolabel" select="'1'"/>
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
<!-- No break after formal object -->
<xsl:param name="formal.object.break.after" select="'0'"/>
<!-- Put each term of a multiple terms variablelistentry on its own line -->
<xsl:param name="variablelist.term.break.after" select="'1'"/>
<!-- Remove the separator between terms -->
<xsl:param name="variablelist.term.separator"></xsl:param>
<!-- Collate copyright years into range -->
<xsl:param name="make.year.ranges" select="1"></xsl:param>

<!-- New in docbook-xsl 1.71.0 -->
<!-- Othercredits contributions inline -->
<xsl:param name="contrib.inline.enabled">1</xsl:param>
<!-- Same style for othercredits as authors style -->
<xsl:param name="othercredit.like.author.enabled">1</xsl:param>
</xsl:stylesheet>
