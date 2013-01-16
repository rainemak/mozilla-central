/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define LOG_COMPONENT "EmbedLiteViewScrolling"
#include "EmbedLog.h"

#include "EmbedLiteViewScrolling.h"
#include "mozilla/unused.h"
#include "nsIDOMWindowUtils.h"
#include "nsNetUtil.h"
#include "nsIDOMClientRect.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMHTMLLIElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsIDOMHTMLInputElement.h"

using namespace mozilla::layers;

namespace mozilla {
namespace embedlite {

EmbedLiteViewScrolling::EmbedLiteViewScrolling(EmbedLiteViewThreadChild* aView)
  : mView(aView)
  , mGotViewPortUpdate(false)
{
    LOGT();
}

EmbedLiteViewScrolling::~EmbedLiteViewScrolling()
{
    LOGT();
}

void
EmbedLiteViewScrolling::ViewportChange(const FrameMetrics& aMetrics, gfx::Rect cssCompositedRect)
{
    mGotViewPortUpdate = true;
    mViewport = gfx::Rect(aMetrics.mScrollOffset.x, aMetrics.mScrollOffset.y,
                          aMetrics.mViewport.width, aMetrics.mViewport.height);
    mCssCompositedRect = gfx::Rect(aMetrics.mScrollOffset.x, aMetrics.mScrollOffset.y,
                                 cssCompositedRect.width, cssCompositedRect.height);
    mCssPageRect = gfx::Rect(aMetrics.mScrollableRect.x, aMetrics.mScrollableRect.y,
                             aMetrics.mScrollableRect.width, aMetrics.mScrollableRect.height);
}

void
EmbedLiteViewScrolling::GestureDoubleTap(const nsIntPoint& aPoint)
{
    // We haven't received a metrics update yet; don't do anything.
    if (!mGotViewPortUpdate) {
        return;
    }

    nsCOMPtr<nsIDOMElement> element;
    gfxRect retRect(0,0,0,0);
    AnyElementFromPoint(mView->mDOMWindow, aPoint.x, aPoint.y, getter_AddRefs(element));
    if (!element) {
        mView->SendZoomToRect(retRect);
        return;
    }

    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(element);
    NS_ENSURE_TRUE(node, );
    nsCOMPtr<nsIDOMElement> elementtest = element;
    while (elementtest && !ShouldZoomToElement(elementtest)) {
        node->GetParentNode(getter_AddRefs(node));
        elementtest = do_QueryInterface(node);
        if (elementtest) {
            element = elementtest;
        }
    }

    if (!element) {
        mView->SendZoomToRect(gfxRect(0,0,0,0));
    } else {
        ZoomToElement(element, aPoint.y, true, true);
    }
}

bool
EmbedLiteViewScrolling::IsRectZoomedIn(gfx::Rect aRect, gfx::Rect aViewport)
{
    // This function checks to see if the area of the rect visible in the
    // viewport (i.e. the "overlapArea" variable below) is approximately 
    // the max area of the rect we can show.
    gfx::Rect vRect(aViewport);
    gfx::Rect overlap = vRect.Intersect(aRect);
    float overlapArea = overlap.width * overlap.height;
    float availHeight = NS_MIN(aRect.width * vRect.height / vRect.width, aRect.height);
    float showing = overlapArea / (aRect.width * availHeight);
    float ratioW = (aRect.width / vRect.width);
    float ratioH = (aRect.height / vRect.height);

    return (showing > 0.9 && (ratioW > 0.9 || ratioH > 0.9)); 
}

bool
EmbedLiteViewScrolling::ShouldZoomToElement(nsIDOMElement* aElement)
{
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
    NS_ENSURE_TRUE(node, false);

    nsCOMPtr<nsIDOMDocument> document;
    NS_ENSURE_SUCCESS(node->GetOwnerDocument(getter_AddRefs(document)), false);

    nsCOMPtr<nsIDOMWindow> win;
    NS_ENSURE_SUCCESS(document->GetDefaultView(getter_AddRefs(win)), false);

    nsCOMPtr<nsIDOMCSSStyleDeclaration> bW;
    NS_ENSURE_SUCCESS(win->GetComputedStyle(aElement, nsString(), getter_AddRefs(bW)), false);

    nsString display;
    if (NS_SUCCEEDED(bW->GetPropertyValue(NS_LITERAL_STRING("display"), display))) {
        if (display.EqualsLiteral("inline")) {
            return false;
        }
    }
    nsCOMPtr<nsIDOMHTMLLIElement> liel = do_QueryInterface(aElement);
    nsCOMPtr<nsIDOMHTMLLIElement> qoteel = do_QueryInterface(aElement);
    if (liel || qoteel)
        return false;

    return true;
}

void
EmbedLiteViewScrolling::AnyElementFromPoint(nsIDOMWindow* aWindow, double aX, double aY, nsIDOMElement* *aElem)
{
    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(aWindow);
    nsCOMPtr<nsIDOMElement> elem;
    NS_ENSURE_SUCCESS(utils->ElementFromPoint(aX, aY, true, true, getter_AddRefs(elem)), );

    nsCOMPtr<nsIDOMHTMLIFrameElement> elAsIFrame = do_QueryInterface(elem);
    nsCOMPtr<nsIDOMHTMLFrameElement> elAsFrame = do_QueryInterface(elem);
    while (elem && (elAsIFrame || elAsFrame)) {
        nsCOMPtr<nsIDOMClientRect> rect;
        elem->GetBoundingClientRect(getter_AddRefs(rect));
        float left, top;
        rect->GetLeft(&left);
        rect->GetTop(&top);
        aX -= left;
        aY -= top;
        nsCOMPtr<nsIDOMDocument> contentDocument;
        if (!elAsIFrame || NS_FAILED(elAsIFrame->GetContentDocument(getter_AddRefs(contentDocument)))) {
            if (!elAsFrame || NS_FAILED(elAsFrame->GetContentDocument(getter_AddRefs(contentDocument)))) {
                break;
            }
        }
        nsCOMPtr<nsIDOMWindow> newWin;
        contentDocument->GetDefaultView(getter_AddRefs(newWin));
        utils = do_GetInterface(newWin);
        if (NS_FAILED(utils->ElementFromPoint(aX, aY, true, true, getter_AddRefs(elem)))) {
            elem = nullptr;
        } else {
            elAsIFrame = do_QueryInterface(elem);
            elAsFrame = do_QueryInterface(elem);
        }
    }
    if (elem) {
        NS_ADDREF(*aElem = elem);
    }

    return;
}

gfx::Rect
EmbedLiteViewScrolling::GetBoundingContentRect(nsIDOMElement* aElement)
{
    gfx::Rect retRect(0, 0, 0, 0);
    if (!aElement)
      return retRect;

    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
    NS_ENSURE_TRUE(node, retRect);
    
    nsCOMPtr<nsIDOMDocument> document;
    NS_ENSURE_SUCCESS(node->GetOwnerDocument(getter_AddRefs(document)), retRect);
    nsCOMPtr<nsIDOMWindow> newWin;
    NS_ENSURE_SUCCESS(document->GetDefaultView(getter_AddRefs(newWin)), retRect);
    nsCOMPtr<nsIDOMElement> element;
    newWin->GetFrameElement(getter_AddRefs(element));
    while (element) {
        if (NS_FAILED(node->GetOwnerDocument(getter_AddRefs(document))) ||
            NS_FAILED(document->GetDefaultView(getter_AddRefs(newWin))) ||
            NS_FAILED(newWin->GetFrameElement(getter_AddRefs(element)))) {
            element = nullptr;
            break;
        }
    }

    nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(newWin);
    int32_t scrollX = 0, scrollY = 0;
    NS_ENSURE_SUCCESS(utils->GetScrollXY(false, &scrollX, &scrollY), retRect);
    nsCOMPtr<nsIDOMClientRect> r;
    aElement->GetBoundingClientRect(getter_AddRefs(r));

    // step out of iframes and frames, offsetting scroll values
    nsCOMPtr<nsIDOMWindow> itWin;
    NS_ENSURE_SUCCESS(node->GetOwnerDocument(getter_AddRefs(document)), retRect);
    NS_ENSURE_SUCCESS(document->GetDefaultView(getter_AddRefs(itWin)), retRect);
    itWin->GetFrameElement(getter_AddRefs(element));
    while (element && itWin != mView->mDOMWindow) {
        // adjust client coordinates' origin to be top left of iframe viewport
        nsCOMPtr<nsIDOMClientRect> gr;
        element->GetBoundingClientRect(getter_AddRefs(gr));
        float grleft, grtop;
        gr->GetLeft(&grleft);
        gr->GetTop(&grtop);

        nsCOMPtr<nsIDOMCSSStyleDeclaration> bW;
        itWin->GetComputedStyle(element, NS_LITERAL_STRING(""), getter_AddRefs(bW));
        nsString blw, btw;
        bW->GetPropertyValue(NS_LITERAL_STRING("border-left-width"), blw);
        bW->GetPropertyValue(NS_LITERAL_STRING("border-top-width"), btw);
        scrollX += grleft + atoi(NS_ConvertUTF16toUTF8(blw).get());
        scrollY += grtop + atoi(NS_ConvertUTF16toUTF8(btw).get());
        itWin->GetParent(getter_AddRefs(itWin));
        itWin->GetFrameElement(getter_AddRefs(element));
    }

    float rleft = 0, rtop = 0, rwidth = 0, rheight = 0;
    r->GetLeft(&rleft);
    r->GetTop(&rtop);
    r->GetWidth(&rwidth);
    r->GetHeight(&rheight);

    return gfx::Rect(rleft + scrollX,
                     rtop + scrollY,
                     rwidth, rheight);
}

void EmbedLiteViewScrolling::ScrollToFocusedInput(bool aAllowZoom)
{
    nsCOMPtr<nsIDOMElement> focused;
    GetFocusedInput(getter_AddRefs(focused));
    if (focused) {
        // _zoomToElement will handle not sending any message if this input is already mostly filling the screen
        ZoomToElement(focused, -1, false, aAllowZoom);
    }
}

nsresult
EmbedLiteViewScrolling::GetFocusedInput(nsIDOMElement* *aElement,
                                        bool aOnlyInputElements)
{
    nsresult rv;
    nsCOMPtr<nsIDOMDocument> doc;
    rv = mView->mDOMWindow->GetDocument(getter_AddRefs(doc));
    NS_ENSURE_TRUE(doc, rv);

    nsCOMPtr<nsIDOMElement> focused;
    doc->GetActiveElement(getter_AddRefs(focused));

    nsCOMPtr<nsIDOMHTMLIFrameElement> elAsIFrame = do_QueryInterface(focused);
    nsCOMPtr<nsIDOMHTMLFrameElement> elAsFrame = do_QueryInterface(focused);
    while (elAsIFrame || elAsFrame) {
        if (!elAsIFrame || NS_FAILED(elAsIFrame->GetContentDocument(getter_AddRefs(doc)))) {
            if (!elAsFrame || NS_FAILED(elAsFrame->GetContentDocument(getter_AddRefs(doc)))) {
                NS_ERROR("This should not happen");
            }
        }
        doc->GetActiveElement(getter_AddRefs(focused));
        elAsIFrame = do_QueryInterface(focused);
        elAsFrame = do_QueryInterface(focused);
    }
    nsCOMPtr<nsIDOMHTMLInputElement> input = do_QueryInterface(focused);
    if (input) {
        bool isText = false;
        if (NS_SUCCEEDED(input->MozIsTextField(false, &isText)) && isText) {
            NS_ADDREF(*aElement = input);
            return NS_OK;
        }
    }

    if (aOnlyInputElements) {
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIDOMHTMLTextAreaElement> textarea = do_QueryInterface(focused);
    bool IsContentEditable = false;
    if (!textarea) {
        nsCOMPtr<nsIDOMHTMLElement> editDiv = do_QueryInterface(focused);
        if (editDiv) {
            editDiv->GetIsContentEditable(&IsContentEditable);
        }
    }
    if (textarea || IsContentEditable) {
        nsCOMPtr<nsIDOMHTMLBodyElement> body = do_QueryInterface(focused);
        if (body) {
            // we are putting focus into a contentEditable frame. scroll the frame into
            // view instead of the contentEditable document contained within, because that
            // results in a better user experience
            nsCOMPtr<nsIDOMNode> node = do_QueryInterface(focused);
            if (node) {
                node->GetOwnerDocument(getter_AddRefs(doc));
                if (doc) {
                    nsCOMPtr<nsIDOMWindow> newWin;
                    doc->GetDefaultView(getter_AddRefs(newWin));
                    if (newWin) {
                        newWin->GetFrameElement(getter_AddRefs(focused));
                    }
                }
            }
        }
        NS_ADDREF(*aElement = focused);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

/* Zoom to an element, optionally keeping a particular part of it
 * in view if it is really tall.
 */
void EmbedLiteViewScrolling::ZoomToElement(nsIDOMElement* aElement, int aClickY, bool aCanZoomOut, bool aCanZoomIn)
{
    const int margin = 15;
    gfx::Rect clrect = GetBoundingContentRect(aElement);
    gfxRect rect(clrect.x, clrect.y, clrect.width, clrect.height);

    gfx::Rect bRect = gfx::Rect(NS_MAX(mCssPageRect.x, clrect.x - margin),
                                clrect.y,
                                clrect.width + 2 * margin,
                                clrect.height);
    // constrict the rect to the screen's right edge
    bRect.width = NS_MIN(bRect.width, (mCssPageRect.x + mCssPageRect.width) - bRect.x);

    // if the rect is already taking up most of the visible area and is stretching the
    // width of the page, then we want to zoom out instead.
    if (IsRectZoomedIn(bRect, mCssCompositedRect)) {
        mView->SendZoomToRect(gfxRect(0,0,0,0));
        return;
    }

    rect.x = round(bRect.x);
    rect.y = round(bRect.y);
    rect.width = round(bRect.width);
    rect.height = round(bRect.height);

    // if the block we're zooming to is really tall, and the user double-tapped
    // more than a screenful of height from the top of it, then adjust the y-coordinate
    // so that we center the actual point the user double-tapped upon. this prevents
    // flying to the top of a page when double-tapping to zoom in (bug 761721).
    // the 1.2 multiplier is just a little fuzz to compensate for bRect including horizontal
    // margins but not vertical ones.
    float cssTapY = mViewport.y + aClickY;
    if ((bRect.height > rect.height) && (cssTapY > rect.y + (rect.height * 1.2))) {
        rect.y = cssTapY - (rect.height / 2);
    }

    mView->SendZoomToRect(rect);
}

} // namespace embedlite
} // namespace mozilla

