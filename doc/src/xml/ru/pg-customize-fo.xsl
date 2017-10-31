<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:fo="http://www.w3.org/1999/XSL/Format" version="1.0" 
    xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="fo:table-cell/fo:block//text()">
    <xsl:variable name="str1">
<!-- insert &zwsp; after '_' to allow wrapping of strings like 'CURRENT_DEFAULT_TRANSFORM_GROUP' -->
        <xsl:call-template name="replace-string">
            <xsl:with-param name="text" select="."/>
            <xsl:with-param name="replace" select="'_'" />
            <xsl:with-param name="with" select="'_&#8203;'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="str2">
<!-- insert &zwsp; after '(' to allow wrapping of strings like 'round(42.4382, 2)' -->
        <xsl:call-template name="replace-string">
            <xsl:with-param name="text" select="$str1"/>
            <xsl:with-param name="replace" select="'('" />
            <xsl:with-param name="with" select="'(&#8203;'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="str3">
<!-- insert &zwsp; after ',' to allow wrapping of strings like '{red,orange,yellow,green,blue,purple}' -->
        <xsl:call-template name="replace-string">
            <xsl:with-param name="text" select="$str2"/>
            <xsl:with-param name="replace" select="','" />
            <xsl:with-param name="with" select="',&#8203;'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="str4">
<!-- insert &zwsp; after '[' to allow wrapping of strings like 'ARRAY['foo','bar','baz']' -->
        <xsl:call-template name="replace-string">
            <xsl:with-param name="text" select="$str3"/>
            <xsl:with-param name="replace" select="'['" />
            <xsl:with-param name="with" select="'[&#8203;'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="str5">
<!-- insert &zwsp; before '::' to allow wrapping of strings like 'E'\\\\001'::bytea' -->
        <xsl:call-template name="replace-string">
            <xsl:with-param name="text" select="$str4"/>
            <xsl:with-param name="replace" select="'::'" />
            <xsl:with-param name="with" select="'&#8203;::'"/>
        </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="$str3" />
  </xsl:template>
<!--
&#xAD; - soft hyphen adds hyphen when wrapping
&#8203; - zero-width space doesn't add anything
-->

  <!-- remove leading spaces after fo:wrapper https://lists.oasis-open.org/archives/docbook-apps/201006/msg00112.html -->
  <xsl:template match="text()[preceding-sibling::*[1][self::fo:wrapper]]">
    <xsl:call-template name="string-ltrim">
      <xsl:with-param name="string" select="." />
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="replace-string">
    <xsl:param name="text"/>
    <xsl:param name="replace"/>
    <xsl:param name="with"/>
    <xsl:choose>
      <xsl:when test="contains($text, $replace)">
        <xsl:value-of select="substring-before($text,$replace)"/>
        <xsl:value-of select="$with"/>
        <xsl:call-template name="replace-string">
          <xsl:with-param name="text" select="substring-after($text,$replace)"/>
          <xsl:with-param name="replace" select="$replace"/>
          <xsl:with-param name="with" select="$with"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$text"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="fo:block[@linefeed-treatment='preserve']/text()">
    <xsl:choose>
      <xsl:when test="(position() = 1 + count(../@*)) and starts-with(., '&#x0a;')">
        <xsl:value-of select="substring(., 2)" />
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="." />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:variable name="whitespace" select="'&#09;&#10;&#13; '" />
<!-- Strips leading whitespace characters from 'string' -->
  <xsl:template name="string-ltrim">
    <xsl:param name="string" />
    <xsl:param name="trim" select="$whitespace" />

    <xsl:if test="string-length($string) &gt; 0">
        <xsl:choose>
            <xsl:when test="contains($trim, substring($string, 1, 1))">
                <xsl:call-template name="string-ltrim">
                    <xsl:with-param name="string" select="substring($string, 2)" />
                    <xsl:with-param name="trim" select="$trim" />
                </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="$string" />
            </xsl:otherwise>
        </xsl:choose>
    </xsl:if>
  </xsl:template>

</xsl:stylesheet>