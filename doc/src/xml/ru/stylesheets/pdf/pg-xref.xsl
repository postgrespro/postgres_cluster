<?xml version='1.0' encoding='ISO-8859-1'?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                xmlns:l="http://docbook.sourceforge.net/xmlns/l10n/1.0"
                version="1.0">

  <xsl:template match="xref" name="xref">
    <xsl:variable name="targets" select="key('id',@linkend)"/>
    <xsl:variable name="target" select="$targets[1]"/>
    <xsl:variable name="refelem" select="local-name($target)"/>
    <xsl:variable name="role" select="@role"/>
    <xsl:call-template name="check.id.unique">
      <xsl:with-param name="linkend" select="@linkend"/>
    </xsl:call-template>
    <xsl:choose>
      <xsl:when test="$refelem=''">
        <xsl:message>
          <xsl:text>XRef to nonexistent id: </xsl:text>
          <xsl:value-of select="@linkend"/>
        </xsl:message>
        <xsl:text>???</xsl:text>
      </xsl:when>
      <xsl:when test="$target/@xreflabel">
        <fo:basic-link internal-destination="{@linkend}"
                      xsl:use-attribute-sets="xref.properties">
          <xsl:call-template name="xref.xreflabel">
            <xsl:with-param name="target" select="$target"/>
          </xsl:call-template>
        </fo:basic-link>
      </xsl:when>
      <xsl:otherwise>
        <fo:basic-link internal-destination="{@linkend}"
                      xsl:use-attribute-sets="xref.properties">
          <xsl:apply-templates select="$target" mode="xref-to">
            <xsl:with-param name="referrer" select="."/>
            <xsl:with-param name="role" select="$role"/>
            <xsl:with-param name="rform" select="@remap"/>
            <xsl:with-param name="xrefstyle">
              <xsl:value-of select="@xrefstyle"/>
            </xsl:with-param>
          </xsl:apply-templates>
        </fo:basic-link>
      </xsl:otherwise>
    </xsl:choose>
    <!-- Add standard page reference? -->
    <xsl:if test="not(starts-with(normalize-space(@xrefstyle), 'select:') != ''
                  and (contains(@xrefstyle, 'page')
                      or contains(@xrefstyle, 'Page')))
                  and ( $insert.xref.page.number = 'yes'
        or $insert.xref.page.number = '1')
                  or local-name($target) = 'para'">
      <fo:basic-link internal-destination="{@linkend}"
                    xsl:use-attribute-sets="xref.properties">
        <xsl:apply-templates select="$target" mode="page.citation">
          <xsl:with-param name="id" select="@linkend"/>
        </xsl:apply-templates>
      </fo:basic-link>
    </xsl:if>
  </xsl:template>

  <xsl:template match="section|simplesect|sect1|sect2|sect3|sect4|sect5
                      |refsect1|refsect2|refsect3|refsection" mode="xref-to">
    <xsl:param name="referrer"/>
    <xsl:param name="xrefstyle"/>
    <xsl:param name="role"/>
    <xsl:apply-templates select="." mode="object.xref.markup">
      <xsl:with-param name="purpose" select="'xref'"/>
      <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
      <xsl:with-param name="referrer" select="$referrer"/>
      <xsl:with-param name="role" select="$role"/>
    </xsl:apply-templates>
  </xsl:template>

  <xsl:template match="*" mode="object.xref.markup">
    <xsl:param name="purpose"/>
    <xsl:param name="xrefstyle"/>
    <xsl:param name="referrer"/>
    <xsl:param name="role"/>
    <xsl:variable name="template">
      <xsl:choose>
        <xsl:when test="starts-with(normalize-space($xrefstyle), 'select:')">
          <xsl:call-template name="make.gentext.template">
            <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
            <xsl:with-param name="purpose" select="$purpose"/>
            <xsl:with-param name="referrer" select="$referrer"/>
          </xsl:call-template>
        </xsl:when>
        <xsl:when test="starts-with(normalize-space($xrefstyle), 'template:')">
          <xsl:value-of select="substring-after(normalize-space($xrefstyle), 'template:')"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="." mode="object.xref.template">
            <xsl:with-param name="purpose" select="$purpose"/>
            <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
            <xsl:with-param name="referrer" select="$referrer"/>
          </xsl:apply-templates>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <xsl:if test="$template = ''">
      <xsl:message>
        <xsl:text>object.xref.markup: empty xref template</xsl:text>
        <xsl:text> for linkend="</xsl:text>
        <xsl:value-of select="@id"/>
        <xsl:text>" and @xrefstyle="</xsl:text>
        <xsl:value-of select="$xrefstyle"/>
        <xsl:text>"</xsl:text>
      </xsl:message>
    </xsl:if>

    <xsl:call-template name="substitute-markup">
      <xsl:with-param name="purpose" select="$purpose"/>
      <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
      <xsl:with-param name="referrer" select="$referrer"/>
      <xsl:with-param name="template" select="$template"/>
      <xsl:with-param name="role" select="$role"/>
    </xsl:call-template>
  </xsl:template>

  <xsl:template name="add.xref.gramcase">
    <xsl:param name="xreftemplate"/>
    <xsl:param name="gramcase" select="''"/>
    <xsl:choose>
      <xsl:when test="$gramcase = '' or $gramcase = 1">
        <xsl:value-of select="$xreftemplate" />
      </xsl:when>
      <xsl:otherwise>
[[gramcase: <xsl:value-of select="$gramcase" />]]
        <xsl:variable name="xrefstart" select="substring-before(normalize-space($xreftemplate), ' ')" />
        <xsl:variable name="xrefend" select="substring-after(normalize-space($xreftemplate), ' ')" />
        <xsl:value-of select="$xreftemplate" />
<xsl:value-of select="substring-before($xreftemplate, ' ')" />
        <xsl:value-of select="concat($xrefstart, ' ', $xrefend)" />
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template name="substitute-markup-x">
    <xsl:param name="template" select="''"/>
    <xsl:param name="allow-anchors" select="'0'"/>
    <xsl:param name="title" select="''"/>
    <xsl:param name="subtitle" select="''"/>
    <xsl:param name="label" select="''"/>
    <xsl:param name="pagenumber" select="''"/>
    <xsl:param name="purpose"/>
    <xsl:param name="xrefstyle"/>
    <xsl:param name="referrer"/>
    <xsl:param name="role"/>
    <xsl:choose>
      <xsl:when test="contains($template, '%')">
        <xsl:value-of select="substring-before($template, '%')"/>
        <xsl:variable name="candidate"
              select="substring(substring-after($template, '%'), 1, 1)"/>
        <xsl:choose>
          <xsl:when test="$candidate = 't'">
            <xsl:apply-templates select="." mode="insert.title.markup">
              <xsl:with-param name="purpose" select="$purpose"/>
              <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
              <xsl:with-param name="role" select="$role"/>
              <xsl:with-param name="title">
                <xsl:choose>
                  <xsl:when test="$title != ''">
                    <xsl:copy-of select="$title"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:apply-templates select="." mode="title.markup">
                      <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
                    </xsl:apply-templates>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:with-param>
            </xsl:apply-templates>
          </xsl:when>
          <xsl:when test="$candidate = 's'">
            <xsl:apply-templates select="." mode="insert.subtitle.markup">
              <xsl:with-param name="purpose" select="$purpose"/>
              <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
              <xsl:with-param name="subtitle">
                <xsl:choose>
                  <xsl:when test="$subtitle != ''">
                    <xsl:copy-of select="$subtitle"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:apply-templates select="." mode="subtitle.markup">
                      <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
                    </xsl:apply-templates>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:with-param>
            </xsl:apply-templates>
          </xsl:when>
          <xsl:when test="$candidate = 'n'">
            <xsl:apply-templates select="." mode="insert.label.markup">
              <xsl:with-param name="purpose" select="$purpose"/>
              <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
              <xsl:with-param name="label">
                <xsl:choose>
                  <xsl:when test="$label != ''">
                    <xsl:copy-of select="$label"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:apply-templates select="." mode="label.markup"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:with-param>
            </xsl:apply-templates>
          </xsl:when>
          <xsl:when test="$candidate = 'p'">
            <xsl:apply-templates select="." mode="insert.pagenumber.markup">
              <xsl:with-param name="purpose" select="$purpose"/>
              <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
              <xsl:with-param name="pagenumber">
                <xsl:choose>
                  <xsl:when test="$pagenumber != ''">
                    <xsl:copy-of select="$pagenumber"/>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:apply-templates select="." mode="pagenumber.markup"/>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:with-param>
            </xsl:apply-templates>
          </xsl:when>
          <xsl:when test="$candidate = 'd'">
            <xsl:apply-templates select="." mode="insert.direction.markup">
              <xsl:with-param name="purpose" select="$purpose"/>
              <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
              <xsl:with-param name="direction">
                <xsl:choose>
                  <xsl:when test="$referrer">
                    <xsl:variable name="referent-is-below">
                      <xsl:for-each select="preceding::xref">
                        <xsl:if test="generate-id(.) = generate-id($referrer)">1</xsl:if>
                      </xsl:for-each>
                    </xsl:variable>
                    <xsl:choose>
                      <xsl:when test="$referent-is-below = ''">
                        <xsl:call-template name="gentext">
                          <xsl:with-param name="key" select="'above'"/>
                        </xsl:call-template>
                      </xsl:when>
                      <xsl:otherwise>
                        <xsl:call-template name="gentext">
                          <xsl:with-param name="key" select="'below'"/>
                        </xsl:call-template>
                      </xsl:otherwise>
                    </xsl:choose>
                  </xsl:when>
                  <xsl:otherwise>
                    <xsl:message>Attempt to use %d in gentext with no referrer!</xsl:message>
                  </xsl:otherwise>
                </xsl:choose>
              </xsl:with-param>
            </xsl:apply-templates>
          </xsl:when>
          <xsl:when test="$candidate = '%' ">
            <xsl:text>%</xsl:text>
          </xsl:when>
          <xsl:otherwise>
            <xsl:text>%</xsl:text><xsl:value-of select="$candidate"/>
          </xsl:otherwise>
        </xsl:choose>
        <!-- recurse with the rest of the template string -->
        <xsl:variable name="rest"
              select="substring($template,
              string-length(substring-before($template, '%'))+3)"/>
        <xsl:call-template name="substitute-markup">
          <xsl:with-param name="template" select="$rest"/>
          <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
          <xsl:with-param name="title" select="$title"/>
          <xsl:with-param name="subtitle" select="$subtitle"/>
          <xsl:with-param name="label" select="$label"/>
          <xsl:with-param name="pagenumber" select="$pagenumber"/>
          <xsl:with-param name="purpose" select="$purpose"/>
          <xsl:with-param name="xrefstyle" select="$xrefstyle"/>
          <xsl:with-param name="referrer" select="$referrer"/>
          <xsl:with-param name="role" select="$role"/>
        </xsl:call-template>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$template"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <xsl:template match="*" mode="insert.title.markup">
    <xsl:param name="purpose"/>
    <xsl:param name="xrefstyle"/>
    <xsl:param name="title"/>
    <xsl:param name="role"/>
    <xsl:choose>
      <xsl:when test="$purpose = 'xref' and titleabbrev">
        <xsl:apply-templates select="." mode="titleabbrev.markup"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:copy-of select="$title"/>
        <xsl:value-of select="$role"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>