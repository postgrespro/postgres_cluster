<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0"
                xmlns:fo="http://www.w3.org/1999/XSL/Format">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl"/>
<xsl:include href="stylesheet-common.xsl" />

<!-- Including our customized templates -->
<!-- Based on https://github.com/gleu/pgdocs_fr/tree/master/postgresql/stylesheets/pdf/ -->
<xsl:include href="stylesheets/pdf/pg-notm.xsl"/>
<xsl:include href="stylesheets/pdf/pg-index.xsl"/>
<xsl:include href="stylesheets/pdf/pg-pagesetup.xsl"/>
<xsl:include href="stylesheets/pdf/pg-sections.xsl"/>
<!-- <xsl:include href="stylesheets/pdf/pg-admon.xsl"/> -->
<xsl:include href="stylesheets/pdf/pg-mixed.xsl"/>
<xsl:include href="stylesheets/pdf/pg-xref.xsl"/>
<xsl:include href="stylesheets/pdf/pg-function.xsl"/>
<xsl:include href="stylesheets/pdf/titlepage.templates.xsl"/>

<xsl:param name="fop1.extensions" select="1"></xsl:param>
<xsl:param name="draft.mode" select="'no'"></xsl:param>
<xsl:param name="tablecolumns.extension" select="0"></xsl:param>
<xsl:param name="toc.max.depth">3</xsl:param>
<xsl:param name="ulink.footnotes" select="1"></xsl:param>
<xsl:param name="use.extensions" select="1"></xsl:param>
<xsl:param name="variablelist.as.blocks" select="1"></xsl:param>

<xsl:param name="xref.with.number.and.title" select="0"></xsl:param>

<!-- Including callout images -->
<!-- <xsl:param name="callout.graphics.path"
  select="'/usr/share/xml/docbook/stylesheet/docbook-xsl/images/callouts/'"></xsl:param> -->
  <!-- TODO: no hardcoded paths -->

<xsl:attribute-set name="monospace.verbatim.properties"
                   use-attribute-sets="verbatim.properties monospace.properties">
  <xsl:attribute name="wrap-option">wrap</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="nongraphical.admonition.properties">
  <xsl:attribute name="border-style">solid</xsl:attribute>
  <xsl:attribute name="border-width">1pt</xsl:attribute>
  <xsl:attribute name="border-color">black</xsl:attribute>
  <xsl:attribute name="padding-start">12pt</xsl:attribute>
  <xsl:attribute name="padding-end">12pt</xsl:attribute>
  <xsl:attribute name="padding-top">6pt</xsl:attribute>
  <xsl:attribute name="padding-bottom">6pt</xsl:attribute>
</xsl:attribute-set>

<xsl:attribute-set name="admonition.title.properties">
  <xsl:attribute name="text-align">center</xsl:attribute>
</xsl:attribute-set>

<!-- fix missing space after vertical simplelist
     https://github.com/docbook/xslt10-stylesheets/issues/31 -->
<xsl:attribute-set name="normal.para.spacing">
  <xsl:attribute name="space-after.optimum">1em</xsl:attribute>
  <xsl:attribute name="space-after.minimum">0.8em</xsl:attribute>
  <xsl:attribute name="space-after.maximum">1.2em</xsl:attribute>
</xsl:attribute-set>

<!-- Change display of some elements -->

<xsl:template match="command">
  <xsl:call-template name="inline.monoseq"/>
</xsl:template>

<xsl:template match="confgroup" mode="bibliography.mode">
  <fo:inline>
    <xsl:apply-templates select="conftitle/text()" mode="bibliography.mode"/>
    <xsl:text>, </xsl:text>
    <xsl:apply-templates select="confdates/text()" mode="bibliography.mode"/>
    <xsl:value-of select="$biblioentry.item.separator"/>
  </fo:inline>
</xsl:template>

<xsl:template match="isbn" mode="bibliography.mode">
  <fo:inline>
    <xsl:text>ISBN </xsl:text>
    <xsl:apply-templates mode="bibliography.mode"/>
    <xsl:value-of select="$biblioentry.item.separator"/>
  </fo:inline>
</xsl:template>

<!-- Printing Style -->
<xsl:param name="double.sided" select="0"></xsl:param>
<xsl:param name="hyphenate">true</xsl:param>
<xsl:param name="hyphenate.verbatim" select="0"></xsl:param>
<xsl:param name="alignment">justify</xsl:param>

<!-- Font size -->
<xsl:param name="body.font.master">8</xsl:param>
<xsl:param name="body.font.size">10pt</xsl:param>

<!-- TOC stuff -->
<xsl:param name="generate.toc">
book      toc
part      nop
</xsl:param>
<xsl:param name="toc.section.depth">1</xsl:param>
<xsl:param name="generate.section.toc.level" select="-1"></xsl:param>
<xsl:param name="toc.indent.width" select="18"></xsl:param>

<!-- Page number in Xref ?-->
<xsl:param name="insert.xref.page.number">no</xsl:param>

<!-- Prevent duplicate e-mails in the Acknowledgments pages-->
<xsl:param name="ulink.show" select="0"></xsl:param>

<!-- bug fix from <https://sourceforge.net/p/docbook/bugs/1360/#831b> -->

<xsl:template match="varlistentry/term" mode="xref-to">
  <xsl:param name="verbose" select="1"/>
  <xsl:apply-templates mode="no.anchor.mode"/>
</xsl:template>

<!-- include refsects in PDF bookmarks
     (https://github.com/docbook/xslt10-stylesheets/issues/46) -->

<xsl:template match="refsect1|refsect2|refsect3"
              mode="bookmark">

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="bookmark-label">
    <xsl:apply-templates select="." mode="object.title.markup"/>
  </xsl:variable>

  <fo:bookmark internal-destination="{$id}">
    <xsl:attribute name="starting-state">
      <xsl:value-of select="$bookmarks.state"/>
    </xsl:attribute>
    <fo:bookmark-title>
      <xsl:value-of select="normalize-space($bookmark-label)"/>
    </fo:bookmark-title>
    <xsl:apply-templates select="*" mode="bookmark"/>
  </fo:bookmark>
</xsl:template>

</xsl:stylesheet>
