<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version="1.0">

<!-- Не выводить tm после каждого упоминания PostgreSQL -->
<xsl:template match="productname">
  <xsl:call-template name="inline.charseq">
  </xsl:call-template>
</xsl:template>

</xsl:stylesheet>