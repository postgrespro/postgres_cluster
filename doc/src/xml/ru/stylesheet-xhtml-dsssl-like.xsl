<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:l="http://docbook.sourceforge.net/xmlns/l10n/1.0"
                version="1.0"
                xmlns="http://www.w3.org/1999/xhtml"
                exclude-result-prefixes="#default">
<xsl:import href="stylesheet-xhtml-dsssl-like-imports.xsl" />

<!-- ==================================================================== -->
<!-- from xhtml/chunk-common.xsl -->
<!-- align footer with dsssl -->
<xsl:template name="footer.navigation">
  <xsl:param name="prev" select="/foo"/>
  <xsl:param name="next" select="/foo"/>
  <xsl:param name="nav.context"/>

  <xsl:variable name="home" select="/*[1]"/>
  <xsl:variable name="up" select="parent::*"/>

  <xsl:variable name="row1" select="count($prev) &gt; 0                                     or count($up) &gt; 0                                     or count($next) &gt; 0"/>

  <xsl:variable name="row2" select="($prev and $navig.showtitles != 0)                                     or (generate-id($home) != generate-id(.)                                         or $nav.context = 'toc')                                     or ($chunk.tocs.and.lots != 0                                         and $nav.context != 'toc')                                     or ($next and $navig.showtitles != 0)"/>

  <xsl:if test="$suppress.navigation = '0' and $suppress.footer.navigation = '0'">
    <div class="navfooter">
      <xsl:if test="$footer.rule != 0">
        <hr/>
      </xsl:if>

      <xsl:if test="$row1 or $row2">
        <table width="100%" summary="Navigation footer">
          <xsl:if test="$row1">
            <tr>
              <td width="40%" align="{$direction.align.start}">
                <xsl:if test="count($prev)&gt;0">
                  <a accesskey="p">
                    <xsl:attribute name="href">
                      <xsl:call-template name="href.target">
                        <xsl:with-param name="object" select="$prev"/>
                      </xsl:call-template>
                    </xsl:attribute>
                    <xsl:call-template name="navig.content">
                      <xsl:with-param name="direction" select="'prev'"/>
                    </xsl:call-template>
                  </a>
                </xsl:if>
                <xsl:text>&#160;</xsl:text>
              </td>
              <td width="20%" align="center">
                <xsl:choose>
                  <xsl:when test="$home != . or $nav.context = 'toc'">
                    <a accesskey="h">
                      <xsl:attribute name="href">
                        <xsl:call-template name="href.target">
                          <xsl:with-param name="object" select="$home"/>
                        </xsl:call-template>
                      </xsl:attribute>
                      <xsl:call-template name="navig.content">
                        <xsl:with-param name="direction" select="'home'"/>
                      </xsl:call-template>
                    </a>
                    <xsl:if test="$chunk.tocs.and.lots != 0 and $nav.context != 'toc'">
                      <xsl:text>&#160;|&#160;</xsl:text>
                    </xsl:if>
                  </xsl:when>
                  <xsl:otherwise>&#160;</xsl:otherwise>
                </xsl:choose>

                <xsl:if test="$chunk.tocs.and.lots != 0 and $nav.context != 'toc'">
                  <a accesskey="t">
                    <xsl:attribute name="href">
                      <xsl:value-of select="$chunked.filename.prefix"/>
                      <xsl:apply-templates select="/*[1]" mode="recursive-chunk-filename">
                        <xsl:with-param name="recursive" select="true()"/>
                      </xsl:apply-templates>
                      <xsl:text>-toc</xsl:text>
                      <xsl:value-of select="$html.ext"/>
                    </xsl:attribute>
                    <xsl:call-template name="gentext">
                      <xsl:with-param name="key" select="'nav-toc'"/>
                    </xsl:call-template>
                  </a>
                </xsl:if>
              </td>
              <td width="40%" align="{$direction.align.end}">
                <xsl:text>&#160;</xsl:text>
                <xsl:if test="count($next)&gt;0">
                  <a accesskey="n">
                    <xsl:attribute name="href">
                      <xsl:call-template name="href.target">
                        <xsl:with-param name="object" select="$next"/>
                      </xsl:call-template>
                    </xsl:attribute>
                    <xsl:call-template name="navig.content">
                      <xsl:with-param name="direction" select="'next'"/>
                    </xsl:call-template>
                  </a>
                </xsl:if>
              </td>
            </tr>
          </xsl:if>

          <xsl:if test="$row2">
            <tr>
              <td width="40%" align="{$direction.align.start}" valign="top">
                <xsl:if test="$navig.showtitles != 0">
                  <xsl:apply-templates select="$prev" mode="object.title.markup.textonly"/>
                </xsl:if>
                <xsl:text>&#160;</xsl:text>
              </td>
              <td width="20%" align="center">
                <xsl:choose>
                  <xsl:when test="count($up)&gt;0                                   and generate-id($up) != generate-id($home)">
                    <a accesskey="u">
                      <xsl:attribute name="href">
                        <xsl:call-template name="href.target">
                          <xsl:with-param name="object" select="$up"/>
                        </xsl:call-template>
                      </xsl:attribute>
                      <xsl:call-template name="navig.content">
                        <xsl:with-param name="direction" select="'up'"/>
                      </xsl:call-template>
                    </a>
                  </xsl:when>
                  <xsl:otherwise>&#160;</xsl:otherwise>
                </xsl:choose>
              </td>
              <td width="40%" align="{$direction.align.end}" valign="top">
                <xsl:text>&#160;</xsl:text>
                <xsl:if test="$navig.showtitles != 0">
                  <xsl:apply-templates select="$next" mode="object.title.markup.textonly"/>
                </xsl:if>
              </td>
            </tr>
          </xsl:if>
        </table>
      </xsl:if>
    </div>
  </xsl:if>
</xsl:template>

<!-- ==================================================================== -->
<!-- from xhtml/chunk-code.xsl (see stylesheet-xhtml-dsssl-like-imports.xsl) -->
<xsl:template match="chapter|sect1|appendix">
  <xsl:choose>
    <xsl:when test="$onechunk != 0 and parent::*">
      <xsl:apply-imports/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="process-chunk-element"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->
<!-- from xhtml/block.xsl -->
<!--
auth-pg-hba-conf.htm:
          Users sometimes wonder why host names are handled in this
          seemingly complicated way, with two name resolutions including a
          reverse lookup of the client's IP address. This complicates use
->
         Users sometimes wonder why host names are handled in this seemingly
         complicated way, with two name resolutions including a reverse lookup
         of the client's IP address. This complicates use of the feature in case

-->
<xsl:template match="sidebar">
  <table cellpadding="5" border="1">
    <xsl:call-template name="common.html.attributes"/>
  <tr><td>
  <div>
    <xsl:call-template name="common.html.attributes"/>
    <xsl:call-template name="id.attribute"/>
    <xsl:call-template name="anchor"/>
    <xsl:call-template name="sidebar.titlepage"/>
    <xsl:apply-templates/>
  </div>
  </td></tr></table>
</xsl:template>

<!-- ==================================================================== -->
<!-- from common/gentext.xsl -->
<!-- error-style-guide.html:
What Goes Where
->
51.3.1. What Goes Where
-->
<xsl:template match="simplesect"
              mode="object.title.template">
<xsl:text>%n. %t</xsl:text>
</xsl:template>

<!-- ==================================================================== -->
<!-- based on xhtml/component.xsl: <xsl:template match="topic/title|topic/info/title" mode="titlepage.mode" priority="2"> -->
<!-- sql-commands.html:
SQL Commands
->
I. SQL Commands
-->
<xsl:template match="reference/title" mode="titlepage.mode" priority="2">
  <xsl:call-template name="component.title">
    <xsl:with-param name="node" select="ancestor::reference[1]"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="reference"
              mode="object.title.template">
<xsl:text>%n. %t</xsl:text>
</xsl:template>

<!-- ==================================================================== -->
<!-- Custom function for chunk numbering -->
<xsl:template name="footnote.in.chunk.number">
  <xsl:param name="node" select="."/>
  <xsl:param name="footnote" select="."/>

  <xsl:variable name="is.chunk">
    <xsl:call-template name="chunk">
      <xsl:with-param name="node" select="$node"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$is.chunk = 1">
      <xsl:value-of select="count($node//footnote[not(@label)][count(./preceding::node()) &lt;= count($node//footnote[not(@label)][.=$footnote]/preceding::node())])" />
    </xsl:when>
    <xsl:otherwise>
      <xsl:if test="$node/parent::*">
        <xsl:call-template name="footnote.in.chunk.number">
          <xsl:with-param name="node" select="$node/parent::*"/>
          <xsl:with-param name="footnote" select="$footnote"/>
        </xsl:call-template>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- from xhtml/footnote.xsl
Independent footnote numbering inside chunks
error-message-reporting.html:
 [11] %m does not require any corresponding entry in the parameter list for errmsg.
->
 [1] %m does not require any corresponding entry in the parameter list for errmsg.
-->
<xsl:template match="footnote" mode="footnote.number">
  <xsl:choose>
    <xsl:when test="string-length(@label) != 0">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="ancestor::table or ancestor::informaltable">
      <xsl:variable name="tfnum">
        <xsl:number level="any" from="table|informaltable" format="1"/>
      </xsl:variable>

      <xsl:choose>
        <xsl:when test="string-length($table.footnote.number.symbols) &gt;= $tfnum">
          <xsl:value-of select="substring($table.footnote.number.symbols, $tfnum, 1)"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:number level="any" from="table | informaltable" format="{$table.footnote.number.format}"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="fnum"> <!--"count($pfoot) - count($ptfoot) + 1"/> -->
        <xsl:choose>
          <xsl:when test="$onechunk != 0">
            <xsl:variable name="pfoot" select="preceding::footnote[not(@label)]"/>
            <xsl:variable name="ptfoot" select="preceding::table//footnote |                                           preceding::informaltable//footnote"/>
            <xsl:value-of select="count($pfoot) - count($ptfoot) + 1"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:call-template name="footnote.in.chunk.number">
            </xsl:call-template>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="string-length($footnote.number.symbols) &gt;= $fnum">
          <xsl:value-of select="substring($footnote.number.symbols, $fnum, 1)"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:number value="$fnum" format="{$footnote.number.format}"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->
<!-- from xhtml/xref.xsl -->
<!--
tutorial-sql-intro.html:
   written on SQL, including [melt93] and [date97].
->
   written on SQL, including Understanding the New SQL and A Guide
   to the SQL Standard.
-->
<xsl:template match="biblioentry|bibliomixed" mode="xref-to">
  <xsl:param name="referrer"/>
  <xsl:param name="xrefstyle"/>
  <xsl:param name="verbose" select="1"/>
 <!-- handles both biblioentry and bibliomixed -->

  <xsl:choose>
    <xsl:when test="string(.) = ''">
      <xsl:variable name="bib" select="document($bibliography.collection,.)"/>
      <xsl:variable name="id" select="(@id|@xml:id)[1]"/>
      <xsl:variable name="entry" select="$bib/bibliography/                                     *[@id=$id or @xml:id=$id][1]"/>
      <xsl:choose>
        <xsl:when test="$entry">
          <xsl:choose>
            <xsl:when test="$bibliography.numbered != 0">
              <xsl:number from="bibliography" count="biblioentry|bibliomixed" level="any" format="1"/>
            </xsl:when>
            <xsl:when test="local-name($entry/*[1]) = 'abbrev'">
              <xsl:apply-templates select="$entry/*[1]" mode="no.anchor.mode"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:value-of select="(@id|@xml:id)[1]"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message>
            <xsl:text>No bibliography entry: </xsl:text>
            <xsl:value-of select="$id"/>
            <xsl:text> found in </xsl:text>
            <xsl:value-of select="$bibliography.collection"/>
          </xsl:message>
          <xsl:value-of select="(@id|@xml:id)[1]"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$bibliography.numbered != 0">
          <xsl:number from="bibliography" count="biblioentry|bibliomixed" level="any" format="1"/>
        </xsl:when>
        <xsl:when test="local-name(*[1]) = 'abbrev'">
          <xsl:apply-templates select="*[1]" mode="no.anchor.mode"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="normalize-space((title|(biblioset[1]/title))[1])"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- from xhtml/xref.xsl -->
<xsl:template match="biblioentry|bibliomixed" mode="xref-to-prefix">
</xsl:template>

<!-- from xhtml/xref.xsl -->
<xsl:template match="biblioentry|bibliomixed" mode="xref-to-suffix">
</xsl:template>

<!-- ==================================================================== -->
<!-- from common/labels.xsl -->
<!-- Table numbering inside reference
pgbench.html:
Table 237. Automatic Variables
->
Table 1. Automatic Variables
-->
<xsl:template match="figure|table|example" mode="label.markup">
  <xsl:variable name="pchap"
                select="(ancestor::chapter
                        |ancestor::appendix
                        |ancestor::article[ancestor::book])[last()]"/>

  <xsl:variable name="prefix">
    <xsl:if test="count($pchap) &gt; 0">
      <xsl:apply-templates select="$pchap" mode="label.markup"/>
    </xsl:if>
  </xsl:variable>
  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$prefix != ''">
            <xsl:apply-templates select="$pchap" mode="label.markup"/>
            <xsl:apply-templates select="$pchap" mode="intralabel.punctuation">
              <xsl:with-param name="object" select="."/>
            </xsl:apply-templates>
          <xsl:number format="1" from="chapter|appendix" level="any"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:number format="1" from="book|article|reference" level="any"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
