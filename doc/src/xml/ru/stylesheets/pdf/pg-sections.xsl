<?xml version='1.0' encoding='ISO-8859-1'?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version="1.0">

     <!-- Force section1's onto a new page -->
<!--
  <xsl:attribute-set name="section.level1.properties">
    <xsl:attribute name="break-after">
      <xsl:choose>
        <xsl:when test="not(position()=last())">
          <xsl:text>page</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:text>auto</xsl:text>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:attribute>
  </xsl:attribute-set>
-->

    <!-- Sections numbering -->
  <xsl:param name="section.autolabel" select="1"/>
  <xsl:param name="section.label.includes.component.label" select="1"/>

    <!-- Skip numeraration for sections with empty title -->
  <xsl:template match="sect2|sect3|sect4|sect5" mode="label.markup">
    <xsl:if test="string-length(title) > 0">
      <!-- label the parent -->
      <xsl:variable name="parent.label">
        <xsl:apply-templates select=".." mode="label.markup"/>
      </xsl:variable>
      <xsl:if test="$parent.label != ''">
        <xsl:apply-templates select=".." mode="label.markup"/>
      <xsl:apply-templates select=".." mode="intralabel.punctuation"/>
      </xsl:if>
      <xsl:choose>
        <xsl:when test="@label">
          <xsl:value-of select="@label"/>
        </xsl:when>
        <xsl:when test="$section.autolabel != 0">
          <xsl:choose>
            <xsl:when test="local-name(.) = 'sect2'">
              <xsl:choose>
                <!-- If the first sect2 isn't numbered, renumber the remainig sections -->
                <xsl:when test="string-length(../sect2[1]/title) = 0">
                  <xsl:variable name="totalsect2">
                    <xsl:number count="sect2"/>
                  </xsl:variable>
                  <xsl:number value="$totalsect2 - 1"/>
                </xsl:when>
                <xsl:otherwise>
                  <xsl:number count="sect2"/>
                </xsl:otherwise>
              </xsl:choose>
            </xsl:when>
            <xsl:when test="local-name(.) = 'sect3'">
              <xsl:number count="sect3"/>
            </xsl:when>
            <xsl:when test="local-name(.) = 'sect4'">
              <xsl:number count="sect4"/>
            </xsl:when>
            <xsl:when test="local-name(.) = 'sect5'">
              <xsl:number count="sect5"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:message>label.markup: this can't happen!</xsl:message>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
      </xsl:choose>
    </xsl:if>
  </xsl:template>

  <!-- Drop the trailing punctuation if title is empty -->
  <xsl:template match="section|sect1|sect2|sect3|sect4|sect5|simplesect
                      |bridgehead"
                mode="object.title.template">
    <xsl:choose>
      <xsl:when test="$section.autolabel != 0">
        <xsl:if test="string-length(title) > 0">
          <xsl:call-template name="gentext.template">
            <xsl:with-param name="context" select="'title-numbered'"/>
            <xsl:with-param name="name">
              <xsl:call-template name="xpath.location"/>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="gentext.template">
          <xsl:with-param name="context" select="'title-unnumbered'"/>
          <xsl:with-param name="name">
            <xsl:call-template name="xpath.location"/>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
