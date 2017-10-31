<?xml version='1.0' encoding='ISO-8859-1'?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version="1.0">

    <!-- Header -->
<!--
  <xsl:template name="header.content">
    <xsl:param name="sequence" select="''"/>
    <fo:block>
      <xsl:attribute name="text-align">
        <xsl:choose>
          <xsl:when test="$sequence = 'first' or $sequence = 'odd'">right</xsl:when>
          <xsl:otherwise>left</xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:value-of select="/book/bookinfo/title"/>
      <xsl:text> - </xsl:text>
      <xsl:value-of select="/book/bookinfo/subtitle"/>
    </fo:block>
  </xsl:template>

  <xsl:template name="header.table">
    <xsl:param name="sequence" select="''"/>
    <xsl:param name="gentext-key" select="''"/>
    <xsl:choose>
      <xsl:when test="$gentext-key = 'book' or $sequence = 'blank'"/>
      <xsl:otherwise>
        <xsl:call-template name="header.content">
          <xsl:with-param name="sequence" select="$sequence"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
-->
<!--
<xsl:template name="header.content">  
  <xsl:param name="pageclass" select="''"/>
  <xsl:param name="sequence" select="''"/>
  <xsl:param name="position" select="''"/>
  <xsl:param name="gentext-key" select="''"/>

  <xsl:variable name="candidate">-->
    <!-- sequence can be odd, even, first, blank -->
    <!-- position can be left, center, right -->
<!--    <xsl:choose>

      <xsl:when test="$sequence = 'odd' and $position = 'left'">
        <fo:retrieve-marker retrieve-class-name="section.head.marker"
                            retrieve-position="first-including-carryover"
                            retrieve-boundary="page-sequence"/>
      </xsl:when>

      <xsl:when test="$sequence = 'odd' and $position = 'center'">
        <xsl:call-template name="draft.text"/>
      </xsl:when>

      <xsl:when test="$sequence = 'odd' and $position = 'right'">
        <fo:page-number/>
      </xsl:when>

      <xsl:when test="$sequence = 'even' and $position = 'left'">  
        <fo:page-number/>
      </xsl:when>

      <xsl:when test="$sequence = 'even' and $position = 'center'">
        <xsl:call-template name="draft.text"/>
      </xsl:when>

      <xsl:when test="$sequence = 'even' and $position = 'right'">
        <xsl:apply-templates select="." mode="titleabbrev.markup"/>
      </xsl:when>

      <xsl:when test="$sequence = 'first' and $position = 'left'">
      </xsl:when>

      <xsl:when test="$sequence = 'first' and $position = 'right'">  
      </xsl:when>

      <xsl:when test="$sequence = 'first' and $position = 'center'"> 
        <xsl:value-of 
               select="ancestor-or-self::book/bookinfo/corpauthor"/>
      </xsl:when>

      <xsl:when test="$sequence = 'blank' and $position = 'left'">
        <fo:page-number/>
      </xsl:when>

      <xsl:when test="$sequence = 'blank' and $position = 'center'">
        <xsl:text>This page intentionally left blank</xsl:text>
      </xsl:when>

      <xsl:when test="$sequence = 'blank' and $position = 'right'">
      </xsl:when>

    </xsl:choose>
  </xsl:variable>

</xsl:template>
-->

    <!-- Centered titles for book and part -->
  <xsl:template name="book.titlepage">
    <fo:block space-before="2in">
      <fo:block>
        <xsl:call-template name="book.titlepage.before.recto"/>
        <xsl:call-template name="book.titlepage.recto"/>
      </fo:block>
      <fo:block>
        <xsl:call-template name="book.titlepage.before.verso"/>
        <xsl:call-template name="book.titlepage.verso"/>
      </fo:block>
      <xsl:call-template name="book.titlepage.separator"/>
    </fo:block>
  </xsl:template>

  <xsl:template name="part.titlepage">
    <fo:block>
      <fo:block space-before="2.5in">
        <xsl:call-template name="part.titlepage.before.recto"/>
        <xsl:call-template name="part.titlepage.recto"/>
      </fo:block>
      <fo:block>
        <xsl:call-template name="part.titlepage.before.verso"/>
        <xsl:call-template name="part.titlepage.verso"/>
      </fo:block>
      <xsl:call-template name="part.titlepage.separator"/>
    </fo:block>
  </xsl:template>

    <!-- Font size for chapter title. -->
  <xsl:template match="title" mode="chapter.titlepage.recto.auto.mode">
    <fo:block xmlns:fo="http://www.w3.org/1999/XSL/Format"
            xsl:use-attribute-sets="chapter.titlepage.recto.style"
            font-size="21pt" font-weight="bold" text-align="left">
      <xsl:call-template name="component.title">
        <xsl:with-param name="node" select="ancestor-or-self::chapter[1]"/>
      </xsl:call-template>
    </fo:block>
  </xsl:template>

    <!-- Margins -->
  <xsl:param name="page.margin.inner">0.5in</xsl:param>
  <xsl:param name="page.margin.outer">0.375in</xsl:param>
  <xsl:param name="body.start.indent" select="'0.7pc'"/>
  <xsl:param name="title.margin.left">-0.7pc</xsl:param>
  <xsl:attribute-set name="normal.para.spacing">
    <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
    <xsl:attribute name="space-before.minimum">0.4em</xsl:attribute>
    <xsl:attribute name="space-before.maximum">0.8em</xsl:attribute>
  </xsl:attribute-set>
  <xsl:attribute-set name="list.block.spacing">
    <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
    <xsl:attribute name="space-before.minimum">0.4em</xsl:attribute>
    <xsl:attribute name="space-before.maximum">0.8em</xsl:attribute>
    <xsl:attribute name="space-after.optimum">0.6em</xsl:attribute>
    <xsl:attribute name="space-after.minimum">0.4em</xsl:attribute>
    <xsl:attribute name="space-after.maximum">0.8em</xsl:attribute>
  </xsl:attribute-set>
  <xsl:attribute-set name="list.item.spacing">
    <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
    <xsl:attribute name="space-before.minimum">0.4em</xsl:attribute>
    <xsl:attribute name="space-before.maximum">0.8em</xsl:attribute>
  </xsl:attribute-set>
  <xsl:attribute-set name="verbatim.properties">
    <xsl:attribute name="space-before.optimum">0.6em</xsl:attribute>
    <xsl:attribute name="space-before.minimum">0.4em</xsl:attribute>
    <xsl:attribute name="space-before.maximum">0.8em</xsl:attribute>
    <xsl:attribute name="space-after.optimum">0.6em</xsl:attribute>
    <xsl:attribute name="space-after.minimum">0.4em</xsl:attribute>
    <xsl:attribute name="space-after.maximum">0.8em</xsl:attribute>
  </xsl:attribute-set>

    <!-- Others-->
  <xsl:param name="header.rule" select="1"></xsl:param>
  <xsl:param name="footer.rule" select="1"></xsl:param>
  <xsl:param name="marker.section.level" select="2"></xsl:param>

    <!-- Dropping a blank page -->
  <xsl:template name="book.titlepage.separator"/>

<xsl:template name="head.sep.rule">
  <xsl:if test="$header.rule != 0">
    <xsl:attribute name="border-bottom-width">0.5pt</xsl:attribute>
    <xsl:attribute name="border-bottom-style">solid</xsl:attribute>
    <xsl:attribute name="border-bottom-color">black</xsl:attribute>
  </xsl:if>
</xsl:template>

<xsl:template name="foot.sep.rule">
  <xsl:if test="$footer.rule != 0">
    <xsl:attribute name="border-top-width">0.5pt</xsl:attribute>
    <xsl:attribute name="border-top-style">solid</xsl:attribute>
    <xsl:attribute name="border-top-color">black</xsl:attribute>
  </xsl:if>
</xsl:template>


</xsl:stylesheet>
