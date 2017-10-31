<?xml version="1.0" encoding="ASCII"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml" version="1.0">

<!-- Add TOC to sect1 -->

<!-- from xhtml/sections.xsl -->

<xsl:template match="sect1">
  <xsl:call-template name="id.warning"/>

  <xsl:element name="{$div.element}" namespace="http://www.w3.org/1999/xhtml">
    <xsl:call-template name="common.html.attributes">
      <xsl:with-param name="inherit" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="id.attribute">
      <xsl:with-param name="conditional" select="0"/>
    </xsl:call-template>

    <xsl:choose>
      <xsl:when test="@renderas = 'sect2'">
        <xsl:call-template name="sect2.titlepage"/>
      </xsl:when>
      <xsl:when test="@renderas = 'sect3'">
        <xsl:call-template name="sect3.titlepage"/>
      </xsl:when>
      <xsl:when test="@renderas = 'sect4'">
        <xsl:call-template name="sect4.titlepage"/>
      </xsl:when>
      <xsl:when test="@renderas = 'sect5'">
        <xsl:call-template name="sect5.titlepage"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="sect1.titlepage"/>
      </xsl:otherwise>
    </xsl:choose>

    <xsl:variable name="toc.params">
      <xsl:call-template name="find.path.params">
        <xsl:with-param name="table" select="normalize-space($generate.toc)"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:if test="contains($toc.params, 'toc')                   and ($generate.section.toc.level &gt;= 1 or count(refentry) &gt; 0)">
      <xsl:call-template name="section.toc">
        <xsl:with-param name="toc.title.p" select="contains($toc.params, 'title')"/>
      </xsl:call-template>
      <xsl:call-template name="section.toc.separator"/>
    </xsl:if>
    <xsl:apply-templates/>
    <xsl:call-template name="process.chunk.footnotes"/>
  </xsl:element>
</xsl:template>

</xsl:stylesheet>
