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
#include "xpcprivate.h"
#include "XPCQuickStubs.h"
#include "nsJSUtils.h"

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
    nsRefPtr<nsHashPropertyBag> hpb = new nsHashPropertyBag();
    if (!hpb)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = hpb->Init();
    if (NS_FAILED(rv)) {
        return rv;
    }

    *aObject = hpb.forget().get();
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
EmbedLiteJSON::ParseJSON(nsAString const& aJson, nsIPropertyBag2** aRoot)
{
    XPCJSContextStack* stack = XPCJSRuntime::Get()->GetJSContextStack();
    JSContext*cx = stack->GetSafeJSContext();
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

    nsCOMPtr<nsIWritablePropertyBag2> contextProps;
    CreateObject(getter_AddRefs(contextProps));

    JSObject& opts = json.toObject();
    JS::AutoIdArray props(cx, JS_Enumerate(cx, &opts));
    for (size_t i = 0; !!props && i < props.length(); ++i) {
        jsid propid = props[i];
        jsval propname, propval;
        if (!JS_IdToValue(cx, propid, &propname) ||
            !JS_GetPropertyById(cx, &opts, propid, &propval)) {
            return NS_ERROR_FAILURE;
        }

        JSString *propnameString = JS_ValueToString(cx, propname);
        nsDependentJSString pstr;
        if (!propnameString || !pstr.init(cx, propnameString)) {
            return NS_ERROR_FAILURE;
        }

        if (JSVAL_IS_BOOLEAN(propval)) {
            contextProps->SetPropertyAsBool(pstr, JSVAL_TO_BOOLEAN(propval));
        } else if (JSVAL_IS_INT(propval)) {
            contextProps->SetPropertyAsInt32(pstr, JSVAL_TO_INT(propval));
        } else if (JSVAL_IS_DOUBLE(propval)) {
            contextProps->SetPropertyAsDouble(pstr, JSVAL_TO_DOUBLE(propval));
        } else if (JSVAL_IS_STRING(propval)) {
            JSString *propvalString = JS_ValueToString(cx, propval);
            nsDependentJSString vstr;
            if (!propvalString || !vstr.init(cx, propvalString)) {
                return NS_ERROR_FAILURE;
            }

            contextProps->SetPropertyAsAString(pstr, vstr);
        }
    }

    *aRoot = contextProps.forget().get();
    return NS_OK;
}

static bool SetPropFromVariant(nsIProperty* aProp, JSContext* aCx, JSObject* aObj)
{
    jsval rval = JSVAL_NULL;
    nsString name;
    nsCOMPtr<nsIVariant> aVariant;
    aProp->GetValue(getter_AddRefs(aVariant));
    aProp->GetName(name);

    XPCCallContext ccx(NATIVE_CALLER, aCx);
    if (!ccx.IsValid())
        return false;
    XPCLazyCallContext lccx(ccx);
    ccx.SetScopeForNewJSObjects(aObj);

    if (!xpc_qsVariantToJsval(lccx, aVariant, &rval)) {
        NS_ERROR("Failed to convert nsIVariant to jsval");
        return false;
    }

    if (!JS_SetProperty(aCx, aObj, NS_ConvertUTF16toUTF8(name).get(), &rval)) {
        NS_ERROR("Failed to set js object property");
        return false;
    }
    return true;
}

NS_IMETHODIMP
EmbedLiteJSON::CreateJSON(nsIPropertyBag *aRoot, nsAString & outJson)
{
    XPCJSContextStack* stack = XPCJSRuntime::Get()->GetJSContextStack();
    JSContext*cx = stack->GetSafeJSContext();
    NS_ENSURE_TRUE(cx, NS_ERROR_FAILURE);

    JSObject* global = JS_GetGlobalObject(cx);
    JSAutoCompartment ac(cx, global);

    JSAutoRequest ar(cx);
    JSObject* obj = JS_NewObject(cx, NULL, NULL, NULL);
    if (!obj)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISimpleEnumerator> windowEnumerator;
    aRoot->GetEnumerator(getter_AddRefs(windowEnumerator));
    bool more;
    windowEnumerator->HasMoreElements(&more);
    while (more) {
        nsCOMPtr<nsIProperty> prop;
        windowEnumerator->GetNext(getter_AddRefs(prop));
        if (prop) {
            SetPropFromVariant(prop, cx, obj);
        }
        windowEnumerator->HasMoreElements(&more);
    }
    jsval vlt = OBJECT_TO_JSVAL(obj);

    NS_ENSURE_TRUE(JS_Stringify(cx, &vlt, nullptr, JSVAL_NULL, JSONCreator, &outJson), NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(!outJson.IsEmpty(), NS_ERROR_FAILURE);

    return NS_OK;
}
