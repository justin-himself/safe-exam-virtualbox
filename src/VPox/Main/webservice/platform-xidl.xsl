<?xml version="1.0"?>

<!--

    platform-xidl.xsl:
        XSLT stylesheet that generates a platform-specific
        VirtualPox.xidl from ../idl/VirtualPox.xidl, which
        is identical to the original except that all <if...>
        sections are resolved (for easier processing).

    Copyright (C) 2006-2020 Oracle Corporation

    This file is part of VirtualPox Open Source Edition (OSE), as
    available from http://www.virtualpox.org. This file is free software;
    you can redistribute it and/or modify it under the terms of the GNU
    General Public License (GPL) as published by the Free Software
    Foundation, in version 2 as it comes in the "COPYING" file of the
    VirtualPox OSE distribution. VirtualPox OSE is distributed in the
    hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
-->

<xsl:stylesheet
  version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output
  method="xml"
  version="1.0"
  encoding="utf-8"
  indent="yes"/>

<xsl:strip-space
  elements="*" />

<!--
    template for "idl" match; this emits the header of the target file
    and recurses into the libraries with interfaces (which are matched below)
    -->
<xsl:template match="/idl">
  <xsl:comment>
  DO NOT EDIT! This is a generated file.
  Generated from: src/VPox/Main/idl/VirtualPox.xidl (VirtualPox's interface definitions in XML)
  Generator: src/VPox/Main/webservice/platform-xidl.xsl
</xsl:comment>

  <idl>
    <xsl:apply-templates />
  </idl>

</xsl:template>

<!--
    template for "if" match: ignore all ifs except those for wsdl
    -->
<xsl:template match="if">
  <xsl:if test="@target='wsdl'">
    <xsl:apply-templates/>
  </xsl:if>
</xsl:template>

<!--
    ignore everything we don't need
    -->
<xsl:template match="cpp|class|enumerator">
</xsl:template>

<!--
    and keep the rest intact (including all attributes)

    NOTE: this drops class and everything in it, which I left unchanged
    since the other xslt scripts blow up badly.
    -->
<xsl:template match="library|module|enum|const|interface|attribute|collection|method|param|result">
  <xsl:copy><xsl:copy-of select="@*"/><xsl:apply-templates/></xsl:copy>
</xsl:template>

<!--
    keep those completely unchanged, including child nodes (including all
    attributes)
    -->
<xsl:template match="descGroup|desc|note">
  <xsl:copy-of select="."/>
</xsl:template>


</xsl:stylesheet>
