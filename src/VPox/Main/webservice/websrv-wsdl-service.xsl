<?xml version="1.0"?>

<!--

    websrv-wsdl.xsl:
        XSLT stylesheet that generates vpoxwebService.wsdl from
        VirtualPox.xidl. That extra WSDL file includes the big
        vpoxweb.wsdl file and adds a "service" section.
        See webservice/Makefile.kmk for an overview of all the things
        generated for the webservice.

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
  targetNamespace="http://schemas.xmlsoap.org/wsdl/"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/">

<xsl:param name="G_argDebug" />

<xsl:output
  method="xml"
  version="1.0"
  encoding="utf-8"
  indent="yes"/>

<xsl:strip-space
  elements="*" />

<!--**********************************************************************
 *
 *  global XSLT variables
 *
 **********************************************************************-->

<xsl:variable name="G_xsltFilename" select="'websrv-wsdl-service.xsl'" />

<xsl:include href="../idl/typemap-shared.inc.xsl" />

<!-- collect all interfaces with "wsmap='suppress'" in a global variable for
     quick lookup -->
<xsl:variable name="G_setSuppressedInterfaces"
              select="//interface[@wsmap='suppress']" />

<!--**********************************************************************
 *
 *  shared helpers
 *
 **********************************************************************-->


<!--**********************************************************************
 *
 *  matches
 *
 **********************************************************************-->

<!--
A WSDL document describes a web service using these major elements:
Element     Defines
<portType>  The operations performed by the web service. A portType can be thought
            of as a class.
<message>   The messages used by the web service. A message is a function call
            and with it come "parts", which are the parameters.
<types>     The data types used by the web service, described in XML Schema
            syntax.
<binding>   The communication protocols used by the web service.

The root tag is  <definitions>.

-->

<xsl:template match="/idl">
  <xsl:comment>
  DO NOT EDIT! This is a generated file.
  Generated from: src/VPox/Main/idl/VirtualPox.xidl (VirtualPox's generic pseudo-IDL file)
  Generator: src/VPox/Main/webservice/websrv-wsdl-service.xsl
</xsl:comment>
  <xsl:apply-templates />
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  if
 - - - - - - - - - - - - - - - - - - - - - - -->

<!--
 *  ignore all |if|s except those for WSDL target
-->
<xsl:template match="if">
  <xsl:if test="@target='wsdl'">
    <xsl:apply-templates/>
  </xsl:if>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  cpp
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="cpp">
<!--  ignore this -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  library
 - - - - - - - - - - - - - - - - - - - - - - -->

<!--
    "library" match: we use this to emit most of the WSDL <types> section.
    With WSDL "document" style, this requires us to go through all interfaces
    and emit complexTypes for all method arguments and return values.
-->
<xsl:template match="library">
  <definitions xmlns:interface="urn:vpox"
               xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/"
               xmlns:vpox="http://www.virtualpox.org/"
               name="VirtualPox"
               xmlns="http://schemas.xmlsoap.org/wsdl/">
    <xsl:attribute name="targetNamespace"><xsl:value-of select="concat($G_targetNamespace, 'Service')" /></xsl:attribute>

    <import location="vpoxweb.wsdl" namespace="urn:vpox">
      <xsl:attribute name="namespace"><xsl:value-of select="$G_targetNamespace" /></xsl:attribute>
    </import>

    <service name="vpoxService">
      <port>
        <xsl:attribute name="binding"><xsl:value-of select="concat('vpox:vpox', $G_bindingSuffix)" /></xsl:attribute>
        <xsl:attribute name="name"><xsl:value-of select="concat('vpox', 'ServicePort')" /></xsl:attribute>
        <soap:address location="http://localhost:18083/"/>
      </port>
    </service>

  </definitions>
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  class
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="module/class">
  <!--  swallow -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  enum
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="enum">
  <!--  swallow -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  const
 - - - - - - - - - - - - - - - - - - - - - - -->

<!--
<xsl:template match="const">
  <xsl:apply-templates />
</xsl:template>
-->

<!-- - - - - - - - - - - - - - - - - - - - - - -
  desc
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="desc">
  <!--  swallow -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  note
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="note">
  <!--  swallow -->
</xsl:template>

<!-- - - - - - - - - - - - - - - - - - - - - - -
     interface
  - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="interface">
</xsl:template>

</xsl:stylesheet>
