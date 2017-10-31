<?xml version="1.0" encoding="ASCII"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml" version="1.0">

<!-- ==================================================================== -->
<!-- from component.xsl -->
<!-- No chapter toc when there is only one sect1
xplang.html:
 Chapter 39. Procedural Languages
   Table of Contents
   [9]39.1. Installing Procedural Languages
->
 Chapter 39. Procedural Languages
-->
<xsl:template match="chapter">
  <xsl:call-template name="id.warning"/>

  <xsl:element name="{$div.element}" namespace="http://www.w3.org/1999/xhtml">
    <xsl:call-template name="common.html.attributes">
      <xsl:with-param name="inherit" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="id.attribute">
      <xsl:with-param name="conditional" select="0"/>
    </xsl:call-template>

    <xsl:call-template name="component.separator"/>
    <xsl:call-template name="chapter.titlepage"/>

    <xsl:variable name="toc.params">
      <xsl:call-template name="find.path.params">
        <xsl:with-param name="table" select="normalize-space($generate.toc)"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:if test="contains($toc.params, 'toc') and count(sect1) &gt; 1">
      <xsl:call-template name="component.toc">
        <xsl:with-param name="toc.title.p" select="contains($toc.params, 'title')"/>
      </xsl:call-template>
      <xsl:call-template name="component.toc.separator"/>
    </xsl:if>
    <xsl:apply-templates/>
    <xsl:call-template name="process.footnotes"/>
  </xsl:element>
</xsl:template>

<!-- ==================================================================== -->
<!-- from sections.xsl -->
<!--
spi-memory.html:
 44.3. Memory Management
->
 44.3. Memory Management
   Table of Contents
   SPI_palloc &mdash; allocate memory in the upper executor context
   SPI_repalloc &mdash; reallocate memory in the upper executor context
-->
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

<!-- ==================================================================== -->
<!-- from xhtml/component.xsl -->
<!--
sourcerepo.html:

Appendix I. The Source Code Repository
   Table of Contents
   [9]I.1. Getting The Source via Git
->
Appendix I. The Source Code Repository
-->
<xsl:template match="appendix">
  <xsl:variable name="ischunk">
    <xsl:call-template name="chunk"/>
  </xsl:variable>

  <xsl:call-template name="id.warning"/>

  <xsl:element name="{$div.element}" namespace="http://www.w3.org/1999/xhtml">
    <xsl:call-template name="common.html.attributes">
      <xsl:with-param name="inherit" select="1"/>
    </xsl:call-template>
    <xsl:call-template name="id.attribute">
      <xsl:with-param name="conditional" select="0"/>
    </xsl:call-template>

    <xsl:choose>
      <xsl:when test="parent::article and $ischunk = 0">
        <xsl:call-template name="section.heading">
          <xsl:with-param name="level" select="1"/>
          <xsl:with-param name="title">
            <xsl:apply-templates select="." mode="object.title.markup"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="component.separator"/>
        <xsl:call-template name="appendix.titlepage"/>
      </xsl:otherwise>
    </xsl:choose>

    <xsl:variable name="toc.params">
      <xsl:call-template name="find.path.params">
        <xsl:with-param name="table" select="normalize-space($generate.toc)"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:if test="contains($toc.params, 'toc') and count(sect1) &gt; 1">
      <xsl:call-template name="component.toc">
        <xsl:with-param name="toc.title.p" select="contains($toc.params, 'title')"/>
      </xsl:call-template>
      <xsl:call-template name="component.toc.separator"/>
    </xsl:if>

    <xsl:apply-templates/>

    <xsl:if test="not(parent::article) or $ischunk != 0">
      <xsl:call-template name="process.footnotes"/>
    </xsl:if>
  </xsl:element>
</xsl:template>

</xsl:stylesheet>
