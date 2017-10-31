<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns="http://www.w3.org/1999/xhtml"
                version='1.0'>

<!-- Performance-optimized versions of some upstream templates from
     epub/ and xhtml-1_1/ directories -->

<!-- Compare with the original template:
wget http://docbook.sourceforge.net/release/xsl/1.79.1/epub/docbook.xsl -O - | grep -Pzo '(?s)<xsl:template match="book[^"]+"\s+mode="ncx">(.*?)</xsl:template>\s' >/tmp/xsl1;
grep -Pzo '(?s)<xsl:template match="book[^"]+"\s+mode="ncx">(.*?)</xsl:template>\s' stylesheet-speedup-epub.xsl >/tmp/xsl2;
diff -u /tmp/xsl1 /tmp/xsl2
-->
<!-- from epub/docbook.xsl -->

  <xsl:template match="book|
                       article|
                       part|
                       reference|
                       preface|
                       chapter|
                       bibliography|
                       appendix|
                       glossary|
                       section|
                       sect1|
                       sect2|
                       sect3|
                       sect4|
                       sect5|
                       refentry|
                       colophon|
                       bibliodiv[title]|
                       setindex|
                       index"
                mode="ncx">
    <xsl:variable name="depth" select="count(ancestor::*)"/>
    <xsl:variable name="title">
      <xsl:if test="$epub.autolabel != 0">
        <xsl:variable name="label.markup">
          <xsl:apply-templates select="." mode="label.markup" />
        </xsl:variable>
        <xsl:if test="normalize-space($label.markup)">
          <xsl:value-of
            select="concat($label.markup,$autotoc.label.separator)" />
        </xsl:if>
      </xsl:if>
      <xsl:apply-templates select="." mode="title.markup" />
    </xsl:variable>

    <xsl:variable name="href">
      <xsl:call-template name="href.target.with.base.dir">
        <xsl:with-param name="context" select="/" />
        <!-- Generate links relative to the location of root file/toc.xml file -->
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="id">
      <xsl:value-of select="generate-id(.)"/>
    </xsl:variable>
    <xsl:variable name="order">
      <!-- Optimization for pgsql-docs: Avoid multiple expensive preceding searches -->
      <xsl:number level="any" count="//book[parent::set]|part|reference|preface|chapter|bibliography|appendix|article|glossary|section|sect1|sect2|sect3|sect4|sect5|refentry|colophon|bibliodiv[title]|setindex|index"/>
<!--
      <xsl:value-of select="$depth +
                                  count(preceding::part|
                                  preceding::reference|
                                  preceding::book[parent::set]|
                                  preceding::preface|
                                  preceding::chapter|
                                  preceding::bibliography|
                                  preceding::appendix|
                                  preceding::article|
                                  preceding::glossary|
                                  preceding::section[not(parent::partintro)]|
                                  preceding::sect1[not(parent::partintro)]|
                                  preceding::sect2[not(ancestor::partintro)]|
                                  preceding::sect3[not(ancestor::partintro)]|
                                  preceding::sect4[not(ancestor::partintro)]|
                                  preceding::sect5[not(ancestor::partintro)]|
                                  preceding::refentry|
                                  preceding::colophon|
                                  preceding::bibliodiv[title]|
                                  preceding::index)"/>
-->
    </xsl:variable>

    <xsl:element name="navPoint" namespace="http://www.daisy.org/z3986/2005/ncx/">
      <xsl:attribute name="id">
        <xsl:value-of select="$id"/>
      </xsl:attribute>

      <xsl:attribute name="playOrder">
        <xsl:choose>
          <xsl:when test="/*[self::set]">
            <xsl:value-of select="$order"/>
          </xsl:when>
          <xsl:when test="$root.is.a.chunk != '0'">
            <xsl:value-of select="$order + 1"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$order - 0"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:attribute>
      <xsl:element name="navLabel" namespace="http://www.daisy.org/z3986/2005/ncx/">
        <xsl:element name="text" namespace="http://www.daisy.org/z3986/2005/ncx/"><xsl:value-of select="normalize-space($title)"/> </xsl:element>
      </xsl:element>
      <xsl:element name="content" namespace="http://www.daisy.org/z3986/2005/ncx/">
        <xsl:attribute name="src">
          <xsl:value-of select="$href"/>
        </xsl:attribute>
      </xsl:element>
      <xsl:apply-templates select="book[parent::set]|part|reference|preface|chapter|bibliography|appendix|article|glossary|section|sect1|sect2|sect3|sect4|sect5|refentry|colophon|bibliodiv[title]|setindex|index" mode="ncx"/>
    </xsl:element>

  </xsl:template>

<!-- Optimization for pgsql-docs: Avoid expensive counting with xsl:number level="any" -->
<!-- Compare with the original template:
wget http://docbook.sourceforge.net/release/xsl/1.79.1/xhtml-1_1/chunk-code.xsl -O - | grep -Pzo '(?s)<xsl:template match="\*" mode="recursive-chunk-filename">(.*?)</xsl:template>\s' >/tmp/xsl1;
grep -Pzo '(?s)<!\-\- modified recursive-chunk-filename template \-\->(.*?)</xsl:template>\s' stylesheet-speedup-epub.xsl >/tmp/xsl2;
diff -u /tmp/xsl1 /tmp/xsl2
-->
<!-- from xhtml-1_1/chunk-code.xsl -->

<!-- modified recursive-chunk-filename template -->
<xsl:key name="kRefentry" match="refentry" use="''"/>
<xsl:key name="kChapter" match="chapter" use="''"/>

<xsl:template match="*" mode="recursive-chunk-filename">
  <xsl:param name="recursive" select="false()"/>
  <xsl:variable name="current-node" select="." />

  <!-- returns the filename of a chunk -->
  <xsl:variable name="ischunk">
    <xsl:call-template name="chunk"/>
  </xsl:variable>

  <xsl:variable name="dbhtml-filename">
    <xsl:call-template name="pi.dbhtml_filename"/>
  </xsl:variable>

  <xsl:variable name="filename">
    <xsl:choose>
      <xsl:when test="$dbhtml-filename != ''">
        <xsl:value-of select="$dbhtml-filename"/>
      </xsl:when>
      <!-- if this is the root element, use the root.filename -->
      <xsl:when test="not(parent::*) and $root.filename != ''">
        <xsl:value-of select="$root.filename"/>
        <xsl:value-of select="$html.ext"/>
      </xsl:when>
      <!-- Special case -->
      <xsl:when test="self::legalnotice and not($generate.legalnotice.link = 0)">
        <xsl:choose>
          <xsl:when test="(@id or @xml:id) and not($use.id.as.filename = 0)">
            <!-- * if this legalnotice has an ID, then go ahead and use -->
            <!-- * just the value of that ID as the basename for the file -->
            <!-- * (that is, without prepending an "ln-" too it) -->
            <xsl:value-of select="(@id|@xml:id)[1]"/>
            <xsl:value-of select="$html.ext"/>
          </xsl:when>
          <xsl:otherwise>
            <!-- * otherwise, if this legalnotice does not have an ID, -->
            <!-- * then we generate an ID... -->
            <xsl:variable name="id">
              <xsl:call-template name="object.id"/>
            </xsl:variable>
            <!-- * ...and then we take that generated ID, prepend an -->
            <!-- * "ln-" to it, and use that as the basename for the file -->
            <xsl:value-of select="concat('ln-',$id,$html.ext)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <!-- if there's no dbhtml filename, and if we're to use IDs as -->
      <!-- filenames, then use the ID to generate the filename. -->
      <xsl:when test="(@id or @xml:id) and $use.id.as.filename != 0">
        <xsl:value-of select="(@id|@xml:id)[1]"/>
        <xsl:value-of select="$html.ext"/>
      </xsl:when>
      <xsl:otherwise/>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$ischunk='0'">
      <!-- if called on something that isn't a chunk, walk up... -->
      <xsl:choose>
        <xsl:when test="count(parent::*)&gt;0">
          <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
            <xsl:with-param name="recursive" select="$recursive"/>
          </xsl:apply-templates>
        </xsl:when>
        <!-- unless there is no up, in which case return "" -->
        <xsl:otherwise/>
      </xsl:choose>
    </xsl:when>

    <xsl:when test="not($recursive) and $filename != ''">
      <!-- if this chunk has an explicit name, use it -->
      <xsl:value-of select="$filename"/>
    </xsl:when>

    <!-- treat nested set separate from root -->
    <xsl:when test="self::set and ancestor::set">
      <xsl:text>se</xsl:text>
      <xsl:number level="any" format="01"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::set">
      <xsl:value-of select="$root.filename"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::book">
      <xsl:text>bk</xsl:text>
      <xsl:number level="any" format="01"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::article">
      <xsl:if test="/set">
        <!-- in a set, make sure we inherit the right book info... -->
        <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
          <xsl:with-param name="recursive" select="true()"/>
        </xsl:apply-templates>
      </xsl:if>

      <xsl:text>ar</xsl:text>
      <xsl:number level="any" format="01" from="book"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::preface">
      <xsl:if test="/set">
        <!-- in a set, make sure we inherit the right book info... -->
        <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
          <xsl:with-param name="recursive" select="true()"/>
        </xsl:apply-templates>
      </xsl:if>

      <xsl:text>pr</xsl:text>
      <xsl:number format="01"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::chapter">
      <xsl:if test="/set">
        <!-- in a set, make sure we inherit the right book info... -->
        <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
          <xsl:with-param name="recursive" select="true()"/>
        </xsl:apply-templates>
      </xsl:if>

      <xsl:text>ch</xsl:text>
      <xsl:for-each select="key('kChapter', '')"><xsl:if test=". = $current-node"><xsl:number format="01" value="position()"/></xsl:if></xsl:for-each>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::appendix">
      <xsl:if test="/set">
        <!-- in a set, make sure we inherit the right book info... -->
        <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
          <xsl:with-param name="recursive" select="true()"/>
        </xsl:apply-templates>
      </xsl:if>

      <xsl:text>ap</xsl:text>
      <xsl:number level="any" format="a" from="book"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::part">
      <xsl:choose>
        <xsl:when test="/set">
          <!-- in a set, make sure we inherit the right book info... -->
          <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
            <xsl:with-param name="recursive" select="true()"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:text>pt</xsl:text>
      <xsl:number format="01"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::reference">
      <xsl:choose>
        <xsl:when test="/set">
          <!-- in a set, make sure we inherit the right book info... -->
          <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
            <xsl:with-param name="recursive" select="true()"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:text>rn</xsl:text>
      <xsl:number format="01"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::refentry">
      <xsl:choose>
        <xsl:when test="parent::reference">
          <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
            <xsl:with-param name="recursive" select="true()"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
          <xsl:if test="/set">
            <!-- in a set, make sure we inherit the right book info... -->
            <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
              <xsl:with-param name="recursive" select="true()"/>
            </xsl:apply-templates>
          </xsl:if>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:text>re</xsl:text>
      <xsl:for-each select="key('kRefentry', '')"><xsl:if test=". = $current-node"><xsl:number format="01" value="position()"/></xsl:if></xsl:for-each>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::colophon">
      <xsl:choose>
        <xsl:when test="/set">
          <!-- in a set, make sure we inherit the right book info... -->
          <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
            <xsl:with-param name="recursive" select="true()"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:text>co</xsl:text>
      <xsl:number level="any" format="01" from="book"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::sect1                     or self::sect2                     or self::sect3                     or self::sect4                     or self::sect5                     or self::section">
      <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
        <xsl:with-param name="recursive" select="true()"/>
      </xsl:apply-templates>
      <xsl:text>s</xsl:text>
      <xsl:number format="01"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::bibliography">
      <xsl:choose>
        <xsl:when test="/set">
          <!-- in a set, make sure we inherit the right book info... -->
          <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
            <xsl:with-param name="recursive" select="true()"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:text>bi</xsl:text>
      <xsl:number level="any" format="01" from="book"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::glossary">
      <xsl:choose>
        <xsl:when test="/set">
          <!-- in a set, make sure we inherit the right book info... -->
          <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
            <xsl:with-param name="recursive" select="true()"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:text>go</xsl:text>
      <xsl:number level="any" format="01" from="book"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::index">
      <xsl:choose>
        <xsl:when test="/set">
          <!-- in a set, make sure we inherit the right book info... -->
          <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
            <xsl:with-param name="recursive" select="true()"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:text>ix</xsl:text>
      <xsl:number format="01"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::setindex">
      <xsl:text>si</xsl:text>
      <xsl:number level="any" format="01" from="set"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:when test="self::topic">
      <xsl:choose>
        <xsl:when test="/set">
          <!-- in a set, make sure we inherit the right book info... -->
          <xsl:apply-templates mode="recursive-chunk-filename" select="parent::*">
            <xsl:with-param name="recursive" select="true()"/>
          </xsl:apply-templates>
        </xsl:when>
        <xsl:otherwise>
        </xsl:otherwise>
      </xsl:choose>

      <xsl:text>to</xsl:text>
      <xsl:number level="any" format="01" from="book"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:when>

    <xsl:otherwise>
      <xsl:text>chunk-filename-error-</xsl:text>
      <xsl:value-of select="name(.)"/>
      <xsl:number level="any" format="01" from="set"/>
      <xsl:if test="not($recursive)">
        <xsl:value-of select="$html.ext"/>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- based on similar optimizations in stylesheet-speedup-xhtml -->
<!-- from xhtml-1_1/autoidx.xsl -->

<xsl:template match="indexterm" mode="reference">
  <xsl:param name="scope" select="."/>
  <xsl:param name="role" select="''"/>
  <xsl:param name="type" select="''"/>
  <xsl:param name="position"/>
  <xsl:param name="separator" select="''"/>
  
  <xsl:variable name="term.separator">
    <xsl:call-template name="index.separator">
      <xsl:with-param name="key" select="'index.term.separator'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="number.separator">
    <xsl:call-template name="index.separator">
      <xsl:with-param name="key" select="'index.number.separator'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="range.separator">
    <xsl:call-template name="index.separator">
      <xsl:with-param name="key" select="'index.range.separator'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$separator != ''">
      <xsl:value-of select="$separator"/>
    </xsl:when>
    <xsl:when test="$position = 1">
      <xsl:value-of select="$term.separator"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$number.separator"/>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:choose>
    <xsl:when test="@zone and string(@zone)">
      <xsl:call-template name="reference">
        <xsl:with-param name="zones" select="normalize-space(@zone)"/>
        <xsl:with-param name="position" select="position()"/>
        <xsl:with-param name="scope" select="$scope"/>
        <xsl:with-param name="role" select="$role"/>
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <a>
        <xsl:apply-templates select="." mode="class.attribute"/>
        <xsl:variable name="title">
          <xsl:choose>
            <xsl:when test="$index.prefer.titleabbrev != 0">
              <xsl:apply-templates select="(ancestor-or-self::set|ancestor-or-self::book|ancestor-or-self::part|ancestor-or-self::reference|ancestor-or-self::partintro|ancestor-or-self::chapter|ancestor-or-self::appendix|ancestor-or-self::preface|ancestor-or-self::article|ancestor-or-self::section|ancestor-or-self::sect1|ancestor-or-self::sect2|ancestor-or-self::sect3|ancestor-or-self::sect4|ancestor-or-self::sect5|ancestor-or-self::refentry|ancestor-or-self::refsect1|ancestor-or-self::refsect2|ancestor-or-self::refsect3|ancestor-or-self::simplesect|ancestor-or-self::bibliography|ancestor-or-self::glossary|ancestor-or-self::index|ancestor-or-self::webpage|ancestor-or-self::topic)[last()]" mode="titleabbrev.markup"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="(ancestor-or-self::set|ancestor-or-self::book|ancestor-or-self::part|ancestor-or-self::reference|ancestor-or-self::partintro|ancestor-or-self::chapter|ancestor-or-self::appendix|ancestor-or-self::preface|ancestor-or-self::article|ancestor-or-self::section|ancestor-or-self::sect1|ancestor-or-self::sect2|ancestor-or-self::sect3|ancestor-or-self::sect4|ancestor-or-self::sect5|ancestor-or-self::refentry|ancestor-or-self::refsect1|ancestor-or-self::refsect2|ancestor-or-self::refsect3|ancestor-or-self::simplesect|ancestor-or-self::bibliography|ancestor-or-self::glossary|ancestor-or-self::index|ancestor-or-self::webpage|ancestor-or-self::topic)[last()]" mode="title.markup"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:variable>

        <xsl:attribute name="href">
          <xsl:choose>
            <xsl:when test="$index.links.to.section = 1">
              <xsl:call-template name="href.target">
                <xsl:with-param name="object" select="(ancestor-or-self::set|ancestor-or-self::book|ancestor-or-self::part|ancestor-or-self::reference|ancestor-or-self::partintro|ancestor-or-self::chapter|ancestor-or-self::appendix|ancestor-or-self::preface|ancestor-or-self::article|ancestor-or-self::section|ancestor-or-self::sect1|ancestor-or-self::sect2|ancestor-or-self::sect3|ancestor-or-self::sect4|ancestor-or-self::sect5|ancestor-or-self::refentry|ancestor-or-self::refsect1|ancestor-or-self::refsect2|ancestor-or-self::refsect3|ancestor-or-self::simplesect|ancestor-or-self::bibliography|ancestor-or-self::glossary|ancestor-or-self::index|ancestor-or-self::webpage|ancestor-or-self::topic)[last()]"/>
                <!-- Optimization for pgsql-docs: We only have an index as a
                     child of book, so look that up directly instead of
                     scanning the entire node tree.  Also, don't look for
                     setindex. -->
                <!-- <xsl:with-param name="context" select="(//index[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))] | //setindex[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))])[1]"/> -->
                <xsl:with-param name="context" select="(/book/index[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))])[1]"/>
              </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
              <xsl:call-template name="href.target">
                <xsl:with-param name="object" select="."/>
                <xsl:with-param name="context" select="(//index[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))] | //setindex[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))])[1]"/>
              </xsl:call-template>
            </xsl:otherwise>
          </xsl:choose>

        </xsl:attribute>

        <xsl:value-of select="$title"/> <!-- text only -->
      </a>

      <xsl:variable name="id" select="(@id|@xml:id)[1]"/>
      <xsl:if test="key('endofrange', $id)[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))]">
        <xsl:apply-templates select="key('endofrange', $id)[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))][last()]" mode="reference">
          <xsl:with-param name="position" select="position()"/>
          <xsl:with-param name="scope" select="$scope"/>
          <xsl:with-param name="role" select="$role"/>
          <xsl:with-param name="type" select="$type"/>
          <xsl:with-param name="separator" select="$range.separator"/>
        </xsl:apply-templates>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="reference">
  <xsl:param name="scope" select="."/>
  <xsl:param name="role" select="''"/>
  <xsl:param name="type" select="''"/>
  <xsl:param name="zones"/>

  <xsl:choose>
    <xsl:when test="contains($zones, ' ')">
      <xsl:variable name="zone" select="substring-before($zones, ' ')"/>
      <xsl:variable name="target" select="key('sections', $zone)"/>

      <a>
        <xsl:apply-templates select="." mode="class.attribute"/>
        <xsl:call-template name="id.attribute"/>
        <xsl:attribute name="href">
          <xsl:call-template name="href.target">
            <xsl:with-param name="object" select="$target[1]"/>
            <xsl:with-param name="context" select="//index[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))][1]"/>
          </xsl:call-template>
        </xsl:attribute>
        <xsl:apply-templates select="$target[1]" mode="index-title-content"/>
      </a>
      <xsl:text>, </xsl:text>
      <xsl:call-template name="reference">
        <xsl:with-param name="zones" select="substring-after($zones, ' ')"/>
        <xsl:with-param name="position" select="position()"/>
        <xsl:with-param name="scope" select="$scope"/>
        <xsl:with-param name="role" select="$role"/>
        <xsl:with-param name="type" select="$type"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="zone" select="$zones"/>
      <xsl:variable name="target" select="key('sections', $zone)"/>

      <a>
        <xsl:apply-templates select="." mode="class.attribute"/>
        <xsl:call-template name="id.attribute"/>
        <xsl:attribute name="href">
          <xsl:call-template name="href.target">
            <xsl:with-param name="object" select="$target[1]"/>
            <!-- Optimization for pgsql-docs: Only look for index under book
                 instead of searching the whole node tree. -->
            <!-- <xsl:with-param name="context" select="//index[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))][1]"/> -->
            <xsl:with-param name="context" select="/book/index[count(ancestor::node()|$scope) = count(ancestor::node()) and ($role = @role or $type = @type or (string-length($role) = 0 and string-length($type) = 0))][1]"/>
          </xsl:call-template>
        </xsl:attribute>
        <xsl:apply-templates select="$target[1]" mode="index-title-content"/>
      </a>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="chunk-first-section-with-parent">
  <xsl:param name="content">
    <xsl:apply-imports/>
  </xsl:param>

  <!-- These xpath expressions are really hairy. The trick is to pick sections -->
  <!-- that are not first children and are not the children of first children -->

  <!-- Optimization for pgsql-docs: Since we set a fixed $chunk.section.depth,
       we can do away with a bunch of complicated XPath searches for the
       previous and next sections at various levels. -->

  <xsl:if test="$chunk.section.depth != 1">
    <xsl:message terminate="yes">
      <xsl:text>Error: If you change $chunk.section.depth, then you must update the performance-optimized chunk-first-section-with-parent template.</xsl:text>
    </xsl:message>
  </xsl:if>

  <xsl:variable name="prev" select="(preceding::book[1]              |preceding::preface[1]              |preceding::chapter[1]              |preceding::appendix[1]              |preceding::part[1]              |preceding::reference[1]              |preceding::refentry[1]              |preceding::colophon[1]              |preceding::article[1]              |preceding::topic[1]              |preceding::bibliography[parent::article or parent::book or parent::part][1]              |preceding::glossary[parent::article or parent::book or parent::part][1]              |preceding::index[$generate.index != 0]                                [parent::article or parent::book or parent::part][1]              |preceding::setindex[$generate.index != 0][1]              |ancestor::set              |ancestor::book[1]              |ancestor::preface[1]              |ancestor::chapter[1]              |ancestor::appendix[1]              |ancestor::part[1]              |ancestor::reference[1]              |ancestor::article[1]              |ancestor::topic[1] | ancestor::sect1[1] | preceding::sect1[1] )[last()]"/>

  <xsl:variable name="next" select="(following::book[1]              |following::preface[1]              |following::chapter[1]              |following::appendix[1]              |following::part[1]              |following::reference[1]              |following::refentry[1]              |following::colophon[1]              |following::bibliography[parent::article or parent::book or parent::part][1]              |following::glossary[parent::article or parent::book or parent::part][1]              |following::index[$generate.index != 0]                                [parent::article or parent::book or parent::part][1]              |following::article[1]              |following::topic[1]              |following::setindex[$generate.index != 0][1]              |descendant::book[1]              |descendant::preface[1]              |descendant::chapter[1]              |descendant::appendix[1]              |descendant::article[1]              |descendant::topic[1]              |descendant::bibliography[parent::article or parent::book or parent::part][1]              |descendant::glossary[parent::article or parent::book or parent::part][1]              |descendant::index[$generate.index != 0]                                [parent::article or parent::book or parent::part][1]              |descendant::colophon[1]              |descendant::setindex[$generate.index != 0][1]              |descendant::part[1]              |descendant::reference[1]              |descendant::refentry[1]              | following::sect1[1] | descendant::sect1[1] )[1]"/>

  <xsl:call-template name="process-chunk">
    <xsl:with-param name="prev" select="$prev"/>
    <xsl:with-param name="next" select="$next"/>
    <xsl:with-param name="content" select="$content"/>
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>
