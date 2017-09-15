<?xml version="1.0"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:exsl="http://exslt.org/common" xmlns:fo="http://www.w3.org/1999/XSL/Format" version="1.0" exclude-result-prefixes="exsl">

<!-- based on templates from docbook/stylesheet/docbook-xsl/fo/titlepage.templates.xsl -->

<xsl:template name="book.titlepage.recto">
<fo:block-container start-indent="5%" end-indent="5%">
  <xsl:choose>
    <xsl:when test="bookinfo/title">
      <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="bookinfo/title"/>
    </xsl:when>
    <xsl:when test="info/title">
      <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="info/title"/>
    </xsl:when>
    <xsl:when test="title">
      <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="title"/>
    </xsl:when>
  </xsl:choose>

  <xsl:choose>
    <xsl:when test="bookinfo/subtitle">
      <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="bookinfo/subtitle"/>
    </xsl:when>
    <xsl:when test="info/subtitle">
      <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="info/subtitle"/>
    </xsl:when>
    <xsl:when test="subtitle">
      <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="subtitle"/>
    </xsl:when>
  </xsl:choose>
</fo:block-container>

<fo:block xsl:use-attribute-sets="book.titlepage.recto.style" space-before="2in"></fo:block>
<fo:block text-align="center">
  <fo:external-graphic src='url("author-logo.svg")' content-height="2in" />
</fo:block>
<fo:block xsl:use-attribute-sets="book.titlepage.recto.style" space-before="4in"></fo:block>

  <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="bookinfo/corpauthor"/>
  <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="info/corpauthor"/>
  <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="bookinfo/authorgroup"/>
  <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="info/authorgroup"/>
  <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="bookinfo/author"/>
  <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="info/author"/>
  <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="bookinfo/itermset"/>
  <xsl:apply-templates mode="book.titlepage.recto.auto.mode" select="info/itermset"/>
<!-- Ad link -->
  <xsl:choose>
    <xsl:when test="contains($pg.productname, 'Postgres Pro') or contains($pg.productname, 'PostgresPro')">
      <xsl:variable name="lang">
        <xsl:call-template name="l10n.language"/>
      </xsl:variable>
      <xsl:variable name="url">
        <xsl:choose>
            <xsl:when test="$lang = 'ru'">https://postgrespro.ru</xsl:when>
            <xsl:otherwise>https://postgrespro.com</xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <fo:block text-align="center">
        <fo:basic-link color="blue" text-decoration="underline">
          <xsl:attribute name="external-destination">url('<xsl:value-of select="$url" />')</xsl:attribute>
          <xsl:value-of select="$url" />
        </fo:basic-link>
      </fo:block>
    </xsl:when>
    <xsl:otherwise></xsl:otherwise>
  </xsl:choose>

</xsl:template>

<xsl:template match="corpauthor" mode="book.titlepage.recto.auto.mode">
<fo:block xsl:use-attribute-sets="book.titlepage.recto.style" font-size="17.28pt" keep-with-next.within-column="always">
<xsl:apply-templates select="." mode="book.titlepage.recto.mode"/>
</fo:block>
</xsl:template>

</xsl:stylesheet>

