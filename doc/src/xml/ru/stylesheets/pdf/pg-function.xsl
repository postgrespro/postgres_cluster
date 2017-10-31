<?xml version='1.0' encoding='ISO-8859-1'?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://www.w3.org/1999/xhtml"
                version="1.0">

<!-- cet astuce est necessaire pour supprimer le ", " ajoute automatiquement -->
<!--
<xsl:template match="function/replaceable" priority="1">
  <xsl:call-template name="inline.italicmonoseq"/>
  <xsl:if test="following-sibling::*">
    <xsl:text></xsl:text>
  </xsl:if>
</xsl:template>
-->

<xsl:template match="function/parameter" priority="1">
  <xsl:call-template name="inline.italicmonoseq"/>
</xsl:template>

<xsl:template match="function/replaceable" priority="1">
  <xsl:call-template name="inline.italicmonoseq"/>
</xsl:template>

</xsl:stylesheet>
