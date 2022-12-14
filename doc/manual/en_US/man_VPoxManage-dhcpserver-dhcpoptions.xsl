<?xml version="1.0"?>
<!--
    Stylesheet that extracts the DHCP option descriptions from
    VirtualPox.xidl for cut & paste into man_VPoxManage-dhcpserver.xml.


    Copyright (C) 2019-2020 Oracle Corporation

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
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  >

  <xsl:output method="text" version="1.0" encoding="utf-8" indent="yes"/>
  <xsl:strip-space elements="*"/>


<!-- Default operation is to supress output -->
<xsl:template match="node()|@*">
  <xsl:apply-templates/>
</xsl:template>

<!--
The work.
-->
<xsl:template mode="emit" match="link[@to='IDHCPServer::networkMask']">
  <xsl:text>the value of the --netmask option</xsl:text>
</xsl:template>

<xsl:template mode="emit" match="link[@to='DHCPOptionEncoding::Hex']">
  <xsl:text>--set-opt-hex</xsl:text>
</xsl:template>

<xsl:template match="desc" mode="emit">
  <xsl:apply-templates mode="emit"/>
</xsl:template>

<xsl:template match="/idl/library/application/enum[@name='DHCPOption']/const">
  <!-- <xsl:message><xsl:text>debug: </xsl:text><xsl:call-template name="get-node-path"/></xsl:message> -->
  <xsl:text>        &lt;varlistentry&gt;
          &lt;term&gt;</xsl:text><xsl:value-of select="concat(@value,' - ',@name)"/><xsl:text>&lt;/term&gt;
          &lt;listitem&gt;&lt;para&gt;</xsl:text>
  <xsl:apply-templates mode="emit"/>
  <xsl:text>&lt;/para&gt;&lt;/listitem&gt;
        &lt;/varlistentry&gt;</xsl:text>
</xsl:template>

<xsl:template match="/">
  <xsl:text>&lt;?xml version="1.0" encoding="UTF-8"?&gt;
&lt;!--
    Manually generated from src/VPox/Main/idl/VirtualPox.xidl by 'kmk dhcpoptions'.
    DO NOT EDIT!


    Copyright (C) 2019-2020 Oracle Corporation

    This file is part of VirtualPox Open Source Edition (OSE), as
    available from http://www.virtualpox.org. This file is free software;
    you can redistribute it and/or modify it under the terms of the GNU
    General Public License (GPL) as published by the Free Software
    Foundation, in version 2 as it comes in the "COPYING" file of the
    VirtualPox OSE distribution. VirtualPox OSE is distributed in the
    hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
--&gt;

      &lt;variablelist&gt;
</xsl:text>
  <xsl:apply-templates/>
  <xsl:text>
      &lt;/variablelist&gt;
</xsl:text>
</xsl:template>


<!--
  Debug/Diagnostics: Return the path to the specified node (by default the current).
  -->
<xsl:template name="get-node-path">
  <xsl:param name="Node" select="."/>
  <xsl:for-each select="$Node">
    <xsl:for-each select="ancestor-or-self::node()">
      <xsl:choose>
        <xsl:when test="name(.) = ''">
          <xsl:text>text()</xsl:text>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="concat('/', name(.))"/>
          <xsl:choose>
            <xsl:when test="@id">
              <xsl:text>[@id=</xsl:text>
              <xsl:value-of select="@id"/>
              <xsl:text>]</xsl:text>
            </xsl:when>
            <xsl:when test="position() > 1">
              <xsl:text>[</xsl:text><xsl:value-of select="position()"/><xsl:text>]</xsl:text>
            </xsl:when>
          </xsl:choose>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>

