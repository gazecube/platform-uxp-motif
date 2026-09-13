/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "nsCOMPtr.h"
#include "mozilla/ModuleUtils.h"

#include "nsIFactory.h"
#include "nsRDFService.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFCompositeDataSource.h"
#include "nsIRDFContentSink.h"
#include "nsISupports.h"
#include "nsRDFBaseDataSources.h"
#include "nsRDFBuiltInDataSources.h"
#include "nsFileSystemDataSource.h"
#include "nsRDFCID.h"
#include "nsIComponentManager.h"
#include "rdf.h"
#include "nsIServiceManager.h"
#include "nsILocalStore.h"
#include "nsRDFXMLParser.h"
#include "nsRDFXMLSerializer.h"

#include "rdfISerializer.h"

//----------------------------------------------------------------------

// Functions used to create new instances of a given object by the
// generic factory.

#define MAKE_CTOR(_func,_new,_ifname)                                \
static nsresult                                                      \
CreateNew##_func(nsISupports* aOuter, REFNSIID aIID, void **aResult) \
{                                                                    \
    if (!aResult) {                                                  \
        return NS_ERROR_INVALID_POINTER;                             \
    }                                                                \
    if (aOuter) {                                                    \
        *aResult = nullptr;                                          \
        return NS_ERROR_NO_AGGREGATION;                              \
    }                                                                \
    nsI##_ifname* inst;                                              \
    nsresult rv = NS_New##_new(&inst);                               \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nullptr;                                          \
        return rv;                                                   \
    }                                                                \
    rv = inst->QueryInterface(aIID, aResult);                        \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nullptr;                                          \
    }                                                                \
    NS_RELEASE(inst);             /* get rid of extra refcnt */      \
    return rv;                                                       \
}

extern nsresult
NS_NewDefaultResource(nsIRDFResource** aResult);

MAKE_CTOR(RDFXMLDataSource,RDFXMLDataSource,RDFDataSource)
MAKE_CTOR(RDFCompositeDataSource,RDFCompositeDataSource,RDFCompositeDataSource)
MAKE_CTOR(RDFContainer,RDFContainer,RDFContainer)

MAKE_CTOR(RDFContainerUtils,RDFContainerUtils,RDFContainerUtils)

MAKE_CTOR(RDFContentSink,RDFContentSink,RDFContentSink)
MAKE_CTOR(RDFDefaultResource,DefaultResource,RDFResource)

#define MAKE_RDF_CTOR(_func,_new,_ifname)                            \
static nsresult                                                      \
CreateNew##_func(nsISupports* aOuter, REFNSIID aIID, void **aResult) \
{                                                                    \
    if (!aResult) {                                                  \
        return NS_ERROR_INVALID_POINTER;                             \
    }                                                                \
    if (aOuter) {                                                    \
        *aResult = nullptr;                                          \
        return NS_ERROR_NO_AGGREGATION;                              \
    }                                                                \
    rdfI##_ifname* inst;                                             \
    nsresult rv = NS_New##_new(&inst);                               \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nullptr;                                          \
        return rv;                                                   \
    }                                                                \
    rv = inst->QueryInterface(aIID, aResult);                        \
    if (NS_FAILED(rv)) {                                             \
        *aResult = nullptr;                                          \
    }                                                                \
    NS_RELEASE(inst);             /* get rid of extra refcnt */      \
    return rv;                                                       \
}

extern nsresult
NS_NewTriplesSerializer(rdfISerializer** aResult);

MAKE_RDF_CTOR(TriplesSerializer, TriplesSerializer, Serializer)

NS_DEFINE_NAMED_CID(NS_RDFCOMPOSITEDATASOURCE_CID);
NS_DEFINE_NAMED_CID(NS_RDFFILESYSTEMDATASOURCE_CID);
NS_DEFINE_NAMED_CID(NS_RDFINMEMORYDATASOURCE_CID);
NS_DEFINE_NAMED_CID(NS_RDFXMLDATASOURCE_CID);
NS_DEFINE_NAMED_CID(NS_RDFDEFAULTRESOURCE_CID);
NS_DEFINE_NAMED_CID(NS_RDFCONTENTSINK_CID);
NS_DEFINE_NAMED_CID(NS_RDFCONTAINER_CID);
NS_DEFINE_NAMED_CID(NS_RDFCONTAINERUTILS_CID);
NS_DEFINE_NAMED_CID(NS_RDFSERVICE_CID);
NS_DEFINE_NAMED_CID(NS_RDFXMLPARSER_CID);
NS_DEFINE_NAMED_CID(NS_RDFXMLSERIALIZER_CID);
NS_DEFINE_NAMED_CID(NS_RDFNTRIPLES_SERIALIZER_CID);
NS_DEFINE_NAMED_CID(NS_LOCALSTORE_CID);

// Mozilla 0.9 Navigator refers to three XPFE data sources that no longer
// exist in UXP. Keep those historic contract names resolvable during the
// frontend bring-up by providing independent empty in-memory graphs. This is
// deliberately only compatibility scaffolding; it does not pretend to
// implement the old bookmark/search services.
static const nsCID kLegacyBookmarksDataSourceCID =
    { 0x4c9f5d8a, 0x6f52, 0x4b6e,
      { 0xa6, 0x93, 0x28, 0x76, 0xcf, 0x08, 0x73, 0xa1 } };
static const nsCID kLegacyLocalSearchDataSourceCID =
    { 0xb51026cd, 0x4aca, 0x49ff,
      { 0x9f, 0x80, 0x44, 0xdf, 0x15, 0xd6, 0x67, 0xce } };
static const nsCID kLegacyInternetSearchDataSourceCID =
    { 0x6d44979f, 0x9a27, 0x4f0e,
      { 0x82, 0x3d, 0xca, 0x32, 0x59, 0xd5, 0x63, 0x5b } };

static const mozilla::Module::CIDEntry kRDFCIDs[] = {
    { &kNS_RDFCOMPOSITEDATASOURCE_CID, false, nullptr, CreateNewRDFCompositeDataSource },
    { &kNS_RDFFILESYSTEMDATASOURCE_CID, false, nullptr, FileSystemDataSource::Create },
    { &kNS_RDFINMEMORYDATASOURCE_CID, false, nullptr, NS_NewRDFInMemoryDataSource },
    { &kLegacyBookmarksDataSourceCID, false, nullptr, NS_NewRDFInMemoryDataSource },
    { &kLegacyLocalSearchDataSourceCID, false, nullptr, NS_NewRDFInMemoryDataSource },
    { &kLegacyInternetSearchDataSourceCID, false, nullptr, NS_NewRDFInMemoryDataSource },
    { &kNS_RDFXMLDATASOURCE_CID, false, nullptr, CreateNewRDFXMLDataSource },
    { &kNS_RDFDEFAULTRESOURCE_CID, false, nullptr, CreateNewRDFDefaultResource },
    { &kNS_RDFCONTENTSINK_CID, false, nullptr, CreateNewRDFContentSink },
    { &kNS_RDFCONTAINER_CID, false, nullptr, CreateNewRDFContainer },
    { &kNS_RDFCONTAINERUTILS_CID, false, nullptr, CreateNewRDFContainerUtils },
    { &kNS_RDFSERVICE_CID, false, nullptr, RDFServiceImpl::CreateSingleton },
    { &kNS_RDFXMLPARSER_CID, false, nullptr, nsRDFXMLParser::Create },
    { &kNS_RDFXMLSERIALIZER_CID, false, nullptr, nsRDFXMLSerializer::Create },
    { &kNS_RDFNTRIPLES_SERIALIZER_CID, false, nullptr, CreateNewTriplesSerializer },
    { &kNS_LOCALSTORE_CID, false, nullptr, NS_NewLocalStore },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kRDFContracts[] = {
    { NS_RDF_DATASOURCE_CONTRACTID_PREFIX "composite-datasource", &kNS_RDFCOMPOSITEDATASOURCE_CID },
    { NS_RDF_DATASOURCE_CONTRACTID_PREFIX "files", &kNS_RDFFILESYSTEMDATASOURCE_CID },
    { NS_RDF_DATASOURCE_CONTRACTID_PREFIX "in-memory-datasource", &kNS_RDFINMEMORYDATASOURCE_CID },
    { NS_RDF_DATASOURCE_CONTRACTID_PREFIX "bookmarks", &kLegacyBookmarksDataSourceCID },
    { NS_RDF_DATASOURCE_CONTRACTID_PREFIX "localsearch", &kLegacyLocalSearchDataSourceCID },
    { NS_RDF_DATASOURCE_CONTRACTID_PREFIX "internetsearch", &kLegacyInternetSearchDataSourceCID },
    { NS_RDF_DATASOURCE_CONTRACTID_PREFIX "xml-datasource", &kNS_RDFXMLDATASOURCE_CID },
    { NS_RDF_RESOURCE_FACTORY_CONTRACTID, &kNS_RDFDEFAULTRESOURCE_CID },
    { NS_RDF_CONTRACTID "/content-sink;1", &kNS_RDFCONTENTSINK_CID },
    { NS_RDF_CONTRACTID "/container;1", &kNS_RDFCONTAINER_CID },
    { NS_RDF_CONTRACTID "/container-utils;1", &kNS_RDFCONTAINERUTILS_CID },
    { NS_RDF_CONTRACTID "/rdf-service;1", &kNS_RDFSERVICE_CID },
    { NS_RDF_CONTRACTID "/xml-parser;1", &kNS_RDFXMLPARSER_CID },
    { NS_RDF_CONTRACTID "/xml-serializer;1", &kNS_RDFXMLSERIALIZER_CID },
    { NS_RDF_SERIALIZER "ntriples", &kNS_RDFNTRIPLES_SERIALIZER_CID },
    { NS_LOCALSTORE_CONTRACTID, &kNS_LOCALSTORE_CID },
    { nullptr }
};

static const mozilla::Module kRDFModule = {
    mozilla::Module::kVersion,
    kRDFCIDs,
    kRDFContracts,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

NSMODULE_DEFN(nsRDFModule) = &kRDFModule;
