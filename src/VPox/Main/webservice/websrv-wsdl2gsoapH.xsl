<?xml version="1.0"?>

<!--
    websrv-gsoapH.xsl:
        XSLT stylesheet that generates a gSOAP pseudo-header
        file from VirtualPox.xidl. Such a pseudo-header files
        can be fed into gSOAP's soapcpp2 to create web service
        client headers and server stubs.
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
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xsd="http://www.w3.org/2001/XMLSchema"
  xmlns:wsdl="http://schemas.xmlsoap.org/wsdl/"
  xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/"
  xmlns:vpox="http://www.virtualpox.org/"
  xmlns:exsl="http://exslt.org/common"
  extension-element-prefixes="exsl"
  >

  <xsl:param name="G_argDebug" />

  <xsl:output method="text"/>

  <xsl:strip-space elements="*"/>

<!-- - - - - - - - - - - - - - - - - - - - - - -
  global XSLT variables
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:variable name="G_xsltFilename" select="'websrv-wsdl2gsoapH.xsl'" />

<xsl:include href="../idl/typemap-shared.inc.xsl" />

<!-- collect all interfaces with "wsmap='suppress'" in a global variable for
     quick lookup -->
<xsl:variable name="G_setSuppressedInterfaces"
              select="//interface[@wsmap='suppress']" />


<!-- - - - - - - - - - - - - - - - - - - - - - -
  Keys for more efficiently looking up of stuff
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:key name="G_keyMessagesByName" match="//wsdl:message[@name]" use="@name"/>
<xsl:key name="G_keySimpleTypesByName" match="//xsd:simpleType[@name]" use="@name"/>
<xsl:key name="G_keyComplexTypesByName" match="//xsd:complexType[@name]" use="@name"/>


<!-- - - - - - - - - - - - - - - - - - - - - - -
  root match
 - - - - - - - - - - - - - - - - - - - - - - -->

<xsl:template match="/wsdl:definitions">
  <xsl:text><![CDATA[
/* DO NOT EDIT! This is a generated file.
 * Generated from: vpoxweb.wsdl (generated WSDL file)
 * Generator: src/VPox/Main/webservice/websrv-gsoapH.xsl
 *
 * Note: This is not a real C/C++ header file. Instead, gSOAP uses files like this
 * one -- with a pseudo-C-header syntax -- to describe a web service API.
 */

// STL vector containers
#import "stlvector.h"

]]></xsl:text>

    <xsl:value-of select="concat('//gsoap vpox  schema namespace: ', $G_targetNamespace)" />
    <xsl:value-of select="concat('//gsoap vpox  schema form:  unqualified', '')" />

  <xsl:text>
/****************************************************************************
 *
 * declarations
 *
 ****************************************************************************/

// forward declarations
class _vpox__InvalidObjectFault;
class _vpox__RuntimeFault;

struct SOAP_ENV__Detail
{
    _vpox__InvalidObjectFault *vpox__InvalidObjectFault;
    _vpox__RuntimeFault *vpox__RuntimeFault;
    int __type;
    void *fault;
    _XML __any;
};
</xsl:text>

  <xsl:apply-templates />
</xsl:template>

<xsl:template name="convertSequence">
  <xsl:param name="xmltype" />
  <xsl:param name="ctype" />

  <xsl:value-of select="concat('class ', $ctype, $G_sNewLine)" />
  <xsl:text>{
    public:
</xsl:text>
  <xsl:for-each select="xsd:element">
    <xsl:variable name="typefield" select="@type" />
    <xsl:variable name="xmltypefield" select="substring($typefield, 5)" /><!-- remove "xsd:" prefix-->
    <xsl:variable name="withoutvpoxtypefield" select="substring($typefield, 6)" /><!-- remove "vpox:" prefix-->
    <xsl:variable name="ctypefield" select="exsl:node-set($G_aSharedTypes)/type[@xmlname=$xmltypefield]/@cname" />
    <xsl:text>        </xsl:text>
    <xsl:choose>
      <xsl:when test="$ctypefield">
        <!-- array or simple type: depends on whether maxOccurs="unbounded" is in WSDL -->
        <xsl:choose>
          <xsl:when test="@maxOccurs='unbounded'">
            <xsl:value-of select="concat('std::vector&lt;', $ctypefield, '&gt;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$ctypefield" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <!-- is there an enum of this type? (look up in simple types) -->
      <xsl:when test="count(key('G_keySimpleTypesByName', $withoutvpoxtypefield)) > 0">
        <xsl:variable name="enumname">
          <xsl:value-of select="concat('enum vpox__', $withoutvpoxtypefield)" />
        </xsl:variable>
        <xsl:choose>
          <xsl:when test="@maxOccurs='unbounded'">
            <xsl:value-of select="concat('std::vector&lt;', $enumname, '&gt;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$enumname" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <!-- is this one of the vpox types? (look up in complex types) -->
      <xsl:when test="count(key('G_keyComplexTypesByName', $withoutvpoxtypefield)) > 0">
        <!-- array or simple type: depends on whether maxOccurs="unbounded" is in WSDL -->
        <xsl:choose>
          <xsl:when test="@maxOccurs='unbounded'">
            <xsl:value-of select="concat('std::vector&lt;vpox__', $withoutvpoxtypefield, '*&gt;')" />
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="concat('vpox__', $withoutvpoxtypefield, '*')" />
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('std::string', '')" />
      </xsl:otherwise>
    </xsl:choose>
    <xsl:variable name="underscoredname">
        <xsl:call-template name="escapeUnderscores">
            <xsl:with-param name="string" select="@name" />
        </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="concat(' ', $underscoredname, ' 1;', $G_sNewLine)" />
  </xsl:for-each>
  <xsl:text>        struct soap *soap;
};
</xsl:text>
<xsl:call-template name="xsltprocNewlineOutputHack"/>

</xsl:template>

<xsl:template match="wsdl:types/xsd:schema">

  <!-- enums are represented as simple types -->
  <xsl:for-each select="xsd:simpleType">
    <xsl:variable name="ctype" select="concat('vpox__', @name)" />
    <xsl:for-each select="xsd:restriction">
      <xsl:value-of select="concat('enum ', $ctype)" />
      <xsl:text>
{
</xsl:text>
      <xsl:for-each select="xsd:enumeration">
        <xsl:variable name="underscoredname">
          <xsl:call-template name="escapeUnderscores">
            <xsl:with-param name="string" select="@value" />
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="concat('    ', $ctype, '__', $underscoredname)" />
        <xsl:if test = "not(position()=last())" >
          <xsl:text >,</xsl:text>
        </xsl:if>
        <xsl:text>
</xsl:text>
      </xsl:for-each>
      <xsl:text>};

</xsl:text>
    </xsl:for-each>
  </xsl:for-each>

  <!-- structs and arrays are represented as complex types -->
  <xsl:for-each select="xsd:complexType">
    <xsl:variable name="xmltype" select="@name" />
    <xsl:variable name="ctype" select="concat('vpox__', $xmltype)" />
    <xsl:for-each select="xsd:sequence">
      <xsl:call-template name="convertSequence">
        <xsl:with-param name="xmltype" select="$xmltype" />
        <xsl:with-param name="ctype" select="$ctype" />
      </xsl:call-template>
    </xsl:for-each>
  </xsl:for-each>

  <!-- individual message elements are represented with xsd:element -> xsd:complexType -> xsdSequence -->
  <xsl:for-each select="xsd:element">
    <xsl:variable name="xmltype" select="@name" />
    <xsl:variable name="underscoredname">
      <xsl:call-template name="escapeUnderscores">
        <xsl:with-param name="string" select="$xmltype" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="ctype" select="concat('_vpox__', $underscoredname)" />
    <xsl:for-each select="xsd:complexType">
      <xsl:for-each select="xsd:sequence">
        <xsl:call-template name="convertSequence">
          <xsl:with-param name="xmltype" select="$xmltype" />
          <xsl:with-param name="ctype" select="$ctype" />
        </xsl:call-template>
      </xsl:for-each>
    </xsl:for-each>
  </xsl:for-each>
</xsl:template>

<xsl:template match="wsdl:portType">

  <xsl:value-of select="concat('//gsoap vpox service name: vpox', $G_bindingSuffix, $G_sNewLine)" />
  <xsl:value-of select="concat('//gsoap vpox service type: vpox', $G_portTypeSuffix, $G_sNewLine)" />
  <xsl:value-of select="concat('//gsoap vpox service namespace: ', $G_targetNamespace, $G_targetNamespaceSeparator, $G_sNewLine)" />
  <xsl:value-of select="concat('//gsoap vpox service transport: ', 'http://schemas.xmlsoap.org/soap/http', $G_sNewLine)" />

  <xsl:for-each select="wsdl:operation">
    <xsl:variable name="methodname" select="@name" />
    <xsl:variable name="cmethodname">
      <xsl:call-template name="escapeUnderscores">
        <xsl:with-param name="string" select="$methodname" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="requestmsg" select="concat($methodname, $G_methodRequest)" />
    <xsl:variable name="responsemsg" select="concat($methodname, $G_methodResponse)" />

    <xsl:value-of select="concat($G_sNewLine, '//gsoap vpox service method-style:    ', $cmethodname, ' ', $G_basefmt)" />
    <xsl:value-of select="concat($G_sNewLine, '//gsoap vpox service method-encoding: ', $cmethodname, ' ', $G_parmfmt)" />
    <xsl:value-of select="concat($G_sNewLine, '//gsoap vpox service method-action:   ', $cmethodname, ' &quot;&quot;')" />
    <xsl:value-of select="concat($G_sNewLine, '//gsoap vpox service method-fault:    ', $cmethodname, ' vpox__InvalidObjectFault')" />
    <xsl:value-of select="concat($G_sNewLine, '//gsoap vpox service method-fault:    ', $cmethodname, ' vpox__RuntimeFault')" />
    <xsl:value-of select="concat($G_sNewLine, 'int __vpox__', $cmethodname, '(', $G_sNewLine)" />

    <!-- request element -->
    <xsl:variable name="reqtype" select="key('G_keyMessagesByName', $requestmsg)/wsdl:part/@element" />
    <xsl:if test="not($reqtype)">
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('wsdl:portType match: Cannot find message with &quot;name&quot;=&quot;', $requestmsg, '&quot;.')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:variable name="creqtype">
      <xsl:call-template name="escapeUnderscores">
        <xsl:with-param name="string" select="substring($reqtype, 6)" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="concat('    _vpox__', $creqtype, '* vpox__', $creqtype, ',', $G_sNewLine)"/>
    <!-- response element -->
    <xsl:variable name="resptype" select="key('G_keyMessagesByName', $responsemsg)/wsdl:part/@element" />
    <xsl:if test="not($resptype)">
      <xsl:call-template name="fatalError">
        <xsl:with-param name="msg" select="concat('wsdl:portType match: Cannot find message with &quot;name&quot;=&quot;', $responsemsg, '&quot;.')" />
      </xsl:call-template>
    </xsl:if>
    <xsl:variable name="cresptype">
      <xsl:call-template name="escapeUnderscores">
        <xsl:with-param name="string" select="substring($resptype, 6)" />
      </xsl:call-template>
    </xsl:variable>
    <xsl:value-of select="concat('    _vpox__', $cresptype, '* vpox__', $cresptype, $G_sNewLine)"/>

    <xsl:text>);</xsl:text>
    <xsl:call-template name="xsltprocNewlineOutputHack"/>

  </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
