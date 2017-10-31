<?xml version='1.0' encoding='ISO-8859-1'?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version="1.0">


    <!-- Graphics in admonitions -->
  <xsl:param name="admon.graphics" select="1"/>
  <xsl:param name="admon.graphics.path"
        select="'img/'"/>

    <!-- Admonition block properties -->
  <xsl:template match="important|warning|caution">
    <xsl:choose>
      <xsl:when test="$admon.graphics != 0">
        <fo:block space-before.minimum="0.4em" space-before.optimum="0.6em"
              space-before.maximum="0.8em" border-style="solid" border-width="1pt"
              border-color="#500" background-color="#FFFFE6">
        <xsl:call-template name="graphical.admonition"/>
        </fo:block>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="nongraphical.admonition"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="note|tip">
    <xsl:choose>
      <xsl:when test="$admon.graphics != 0">
        <fo:block space-before.minimum="0.4em" space-before.optimum="0.6em"
              space-before.maximum="0.8em" border-style="solid" border-width="1pt"
              border-color="#E0E0E0" background-color="#FFFFE6">
        <xsl:call-template name="graphical.admonition"/>
        </fo:block>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="nongraphical.admonition"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

    <!-- Admonitions text properties -->
  <xsl:attribute-set name="admonition.properties">
    <xsl:attribute name="margin-right">6pt</xsl:attribute>
  </xsl:attribute-set>

    <!-- Adding left space to the graphics and color to the titles -->
<!-- Causes overflow
Line 1 of a paragraph overflows the available area by 18000 millipoints. (See position 6:21146)
(<fo:block margin-left="18pt">)
 -->
<!--
  <xsl:template name="graphical.admonition">
    <xsl:variable name="id">
      <xsl:call-template name="object.id"/>
    </xsl:variable>
    <xsl:variable name="graphic.width">
     <xsl:apply-templates select="." mode="admon.graphic.width"/>
    </xsl:variable>
    <fo:block id="{$id}">
      <fo:list-block provisional-distance-between-starts="{$graphic.width} + 18pt"
              provisional-label-separation="18pt" xsl:use-attribute-sets="list.block.spacing">
        <fo:list-item>
            <fo:list-item-label end-indent="label-end()">
              <fo:block margin-left="18pt">
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
                  <xsl:if test="ancestor-or-self::important">
                    <xsl:attribute name="color">#500</xsl:attribute>
                  </xsl:if>
                  <xsl:if test="ancestor-or-self::warning">
                    <xsl:attribute name="color">#500</xsl:attribute>
                  </xsl:if>
                  <xsl:if test="ancestor-or-self::caution">
                    <xsl:attribute name="color">#500</xsl:attribute>
                  </xsl:if>
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
-->
</xsl:stylesheet>
