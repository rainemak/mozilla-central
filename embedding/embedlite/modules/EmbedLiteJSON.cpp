/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EmbedLiteJSON.h"
#include "nsHashPropertyBag.h"
#include "EmbedLiteAppService.h"
#include "nsServiceManagerUtils.h"
#include "jsapi.h"

EmbedLiteJSON::EmbedLiteJSON()
{
}

EmbedLiteJSON::~EmbedLiteJSON()
{
}

NS_IMPL_ISUPPORTS1(EmbedLiteJSON, nsIEmbedLiteJSON)

NS_IMETHODIMP
EmbedLiteJSON::CreateObject(nsIWritablePropertyBag2 * *aObject)
{
    nsHashPropertyBag *hpb = new nsHashPropertyBag();
    if (!hpb)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(hpb);

    nsresult rv = hpb->Init();
    if (NS_FAILED(rv)) {
        NS_RELEASE(hpb);
        return rv;
    }

    *aObject = hpb;
    return NS_OK;
}

static JSBool
JSONCreator(const jschar* aBuf, uint32_t aLen, void* aData)
{
    nsAString* result = static_cast<nsAString*>(aData);
    result->Append(static_cast<const PRUnichar*>(aBuf),
                   static_cast<uint32_t>(aLen));
    return true;
}

NS_IMETHODIMP
EmbedLiteJSON::ParseJSON(unsigned int aWinID, nsAString const& aJson, nsIPropertyBag** aRoot)
{
    nsCOMPtr<nsIEmbedAppService> service = do_GetService("@mozilla.org/embedlite-app-service;1");
    EmbedLiteAppService* intService = static_cast<EmbedLiteAppService*>(service.get());
    JSContext* cx = intService->GetAnyJSContext(aWinID);
    NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);

    JSAutoRequest ar(cx);
    jsval json = JSVAL_NULL;
    if (!JS_ParseJSON(cx,
                      static_cast<const jschar*>(aJson.BeginReading()),
                      aJson.Length(),
                      &json)) {
        NS_ERROR("Failed to parse json string");
        return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsIWritablePropertyBag2> bag;
    CreateObject(getter_AddRefs(bag));
    NS_WARNING("Not implemented conversion from jsval to PropertyBag");

    NS_ADDREF(*aRoot = bag);
    return NS_OK;
}

NS_IMETHODIMP
EmbedLiteJSON::CreateJSON(uint32_t aWinID, nsIPropertyBag *aRoot, nsAString & outJson)
{
    nsCOMPtr<nsIEmbedAppService> service = do_GetService("@mozilla.org/embedlite-app-service;1");
    EmbedLiteAppService* intService = static_cast<EmbedLiteAppService*>(service.get());
    JSContext* cx = intService->GetAnyJSContext(aWinID);
    NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);
    JSAutoRequest ar(cx);
    JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
    if (!obj)
        return NS_ERROR_FAILURE;

    JSAutoCompartment ac(cx, obj);
    jsval v0 = JSVAL_TRUE;
    JS_SetProperty(cx, obj, "here", &v0);
    jsval v1 = JSVAL_FALSE;
    JS_SetProperty(cx, obj, "here2", &v1);
    jsval vlt = OBJECT_TO_JSVAL(obj);

    NS_ENSURE_TRUE(JS_Stringify(cx, &vlt, nullptr, JSVAL_NULL, JSONCreator, &outJson), NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(!outJson.IsEmpty(), NS_ERROR_FAILURE);

    return NS_OK;
}
