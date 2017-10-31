<?xml version='1.0' encoding='UTF-8'?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                               xmlns:l="http://docbook.sourceforge.net/xmlns/l10n/1.0">
<xsl:param name="xref.with.number.and.title" select="0"></xsl:param>
<xsl:variable name="extra.l10n.xml" select="document('l10n.xml')"></xsl:variable>

<xsl:include href="../stylesheet-common.xsl" />

<!-- from gentext.xsl -->
<!-- ============================================================ -->

<xsl:template name="gentext.template">
  <xsl:param name="context" select="'default'"/>
  <xsl:param name="name" select="'default'"/>
  <xsl:param name="origname" select="$name"/>
  <xsl:param name="purpose"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="referrer"/>
  <xsl:param name="lang">
    <xsl:call-template name="l10n.language"/>
  </xsl:param>
  <xsl:param name="verbose" select="1"/>

  <xsl:choose>
    <xsl:when test="false">
      <xsl:for-each select="$l10n.xml">  <!-- We need to switch context in order to make key() work -->
    <xsl:for-each select="document(key('l10n-lang', $lang)/@href)">

      <xsl:variable name="localization.node"
            select="key('l10n-lang', $lang)[1]"/>

      <xsl:if test="count($localization.node) = 0
            and $verbose != 0">
        <xsl:message>
          <xsl:text>No "</xsl:text>
          <xsl:value-of select="$lang"/>
          <xsl:text>" localization exists.</xsl:text>
        </xsl:message>
      </xsl:if>

      <xsl:variable name="context.node"
            select="key('l10n-context', $context)[1]"/>

      <xsl:if test="count($context.node) = 0
            and $verbose != 0">
        <xsl:message>
          <xsl:text>No context named "</xsl:text>
          <xsl:value-of select="$context"/>
          <xsl:text>" exists in the "</xsl:text>
          <xsl:value-of select="$lang"/>
          <xsl:text>" localization.</xsl:text>
        </xsl:message>
      </xsl:if>

      <xsl:for-each select="$context.node">
        <xsl:variable name="template.node"
              select="(key('l10n-template-style', concat($context, '#', $name, '#', $xrefstyle))
                   |key('l10n-template', concat($context, '#', $name)))[1]"/>

        <xsl:choose>
          <xsl:when test="$template.node/@text">
        <xsl:value-of select="$template.node/@text"/>
          </xsl:when>
          <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="contains($name, '/')">
            <xsl:call-template name="gentext.template">
              <xsl:with-param name="context" select="$context"/>
              <xsl:with-param name="name" select="substring-after($name, '/')"/>
              <xsl:with-param name="origname" select="$origname"/>
              <xsl:with-param name="purpose" select="$purpose"/>
              <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
              <xsl:with-param name="referrer" select="$referrer"/>
              <xsl:with-param name="lang" select="$lang"/>
              <xsl:with-param name="verbose" select="$verbose"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:when test="$verbose = 0">
            <!-- silence -->
          </xsl:when>
          <xsl:otherwise>
            <xsl:message>
              <xsl:text>No template for "</xsl:text>
              <xsl:value-of select="$origname"/>
              <xsl:text>" (or any of its leaves) exists in the context named "</xsl:text>
              <xsl:value-of select="$context"/>
              <xsl:text>" in the "</xsl:text>
              <xsl:value-of select="$lang"/>
              <xsl:text>" localization.</xsl:text>
            </xsl:message>
          </xsl:otherwise>
        </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </xsl:for-each>
      </xsl:for-each>
    </xsl:when>
    <xsl:otherwise>
      <xsl:for-each select="$l10n.xml">  <!-- We need to switch context in order to make key() work -->
    <xsl:for-each select="document(key('l10n-lang', $lang)/@href)">

      <xsl:variable name="local.localization.node"
            select="($extra.l10n.xml//l:i18n/l:l10n[@language=$lang])[1]"/>

      <xsl:variable name="localization.node"
            select="key('l10n-lang', $lang)[1]"/>

      <xsl:if test="count($localization.node) = 0
            and count($local.localization.node) = 0
            and $verbose != 0">
        <xsl:message>
          <xsl:text>No "</xsl:text>
          <xsl:value-of select="$lang"/>
          <xsl:text>" localization exists.</xsl:text>
        </xsl:message>
      </xsl:if>

      <xsl:variable name="local.context.node"
            select="$local.localization.node/l:context[@name=$context]"/>

      <xsl:variable name="context.node"
            select="key('l10n-context', $context)[1]"/>

      <xsl:if test="count($context.node) = 0
            and count($local.context.node) = 0
            and $verbose != 0">
        <xsl:message>
          <xsl:text>No context named "</xsl:text>
          <xsl:value-of select="$context"/>
          <xsl:text>" exists in the "</xsl:text>
          <xsl:value-of select="$lang"/>
          <xsl:text>" localization.</xsl:text>
        </xsl:message>
      </xsl:if>

  <!-- LAW  -->

      <xsl:variable name="local.template.node"
            select="($local.context.node/l:template[(
  ($referrer and (not($referrer/@remap) or $referrer/@remap='1') and @name=$name) or
  ($referrer and @name=concat($name, '_', $referrer/@remap)) or
  (not($referrer) and @name=$name)
)
                                and @style
                                and @style=$xrefstyle]
                |$local.context.node/l:template[(
  ($referrer and (not($referrer/@remap) or $referrer/@remap='1') and @name=$name) or
  ($referrer and @name=concat($name, '_', $referrer/@remap)) or
  (not($referrer) and @name=$name)
)
                                and not(@style)])[1]"/>
      <!-- xsl:if test="$local.template.node" -->
      <xsl:if test="not(contains($name, '/')) and $referrer and $referrer/@remap">
        <xsl:if test="not($local.template.node/@text)">
          <xsl:message>
            <xsl:text>No template for: "</xsl:text>
            <xsl:value-of select="concat($name, '_', $referrer/@remap)"/>
            <xsl:text>", context: </xsl:text>
            <xsl:value-of select="$context"/>
            <xsl:text>.</xsl:text>
          </xsl:message>
        </xsl:if>
      </xsl:if>

<!--
        <xsl:message>
          <xsl:text>[[</xsl:text>
          <xsl:value-of select="$local.template.node/@text"/>
        </xsl:message>
-->
<!--
      <xsl:variable name="local.template.node"
            select="($local.context.node/l:template[@name=$name
                                and @style
                                and @style=$xrefstyle]
                |$local.context.node/l:template[@name=$name
                                and not(@style)])[1]"/>
-->
  <!-- LAW -->

      <xsl:for-each select="$context.node">
        <xsl:variable name="template.node"
              select="(key('l10n-template-style', concat($context, '#', $name, '#', $xrefstyle))
                   |key('l10n-template', concat($context, '#', $name)))[1]"/>

        <xsl:choose>
          <xsl:when test="$local.template.node/@text">
        <xsl:value-of select="$local.template.node/@text"/>
          </xsl:when>
          <xsl:when test="$template.node/@text">
        <xsl:value-of select="$template.node/@text"/>
          </xsl:when>
          <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="contains($name, '/')">
            <xsl:call-template name="gentext.template">
              <xsl:with-param name="context" select="$context"/>
              <xsl:with-param name="name" select="substring-after($name, '/')"/>
              <xsl:with-param name="origname" select="$origname"/>
              <xsl:with-param name="purpose" select="$purpose"/>
              <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
              <xsl:with-param name="referrer" select="$referrer"/>
              <xsl:with-param name="lang" select="$lang"/>
              <xsl:with-param name="verbose" select="$verbose"/>
            </xsl:call-template>
          </xsl:when>
          <xsl:when test="$verbose = 0">
            <!-- silence -->
          </xsl:when>
          <xsl:otherwise>
            <xsl:message>
              <xsl:text>No template for "</xsl:text>
              <xsl:value-of select="$origname"/>
              <xsl:text>" (or any of its leaves) exists in the context named "</xsl:text>
              <xsl:value-of select="$context"/>
              <xsl:text>" in the "</xsl:text>
              <xsl:value-of select="$lang"/>
              <xsl:text>" localization.</xsl:text>
            </xsl:message>
          </xsl:otherwise>
        </xsl:choose>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:for-each>
    </xsl:for-each>
      </xsl:for-each>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
