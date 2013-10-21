/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "TextAttrs.h"

#include "Accessible-inl.h"
#include "nsAccUtils.h"
#include "nsCoreUtils.h"
#include "StyleInfo.h"

#include "gfxFont.h"
#include "nsFontMetrics.h"
#include "nsLayoutUtils.h"
#include "HyperTextAccessible.h"
#include "mozilla/AppUnits.h"

using namespace mozilla;
using namespace mozilla::a11y;

////////////////////////////////////////////////////////////////////////////////
// TextAttrsMgr
////////////////////////////////////////////////////////////////////////////////

void
TextAttrsMgr::GetAttributes(nsIPersistentProperties* aAttributes,
                            int32_t* aStartHTOffset,
                            int32_t* aEndHTOffset)
{
  // 1. Hyper text accessible must be specified always.
  // 2. Offset accessible and result hyper text offsets must be specified in
  // the case of text attributes.
  // 3. Offset accessible and result hyper text offsets must not be specified
  // but include default text attributes flag and attributes list must be
  // specified in the case of default text attributes.
  NS_PRECONDITION(mHyperTextAcc &&
                  ((mOffsetAcc && mOffsetAccIdx != -1 &&
                    aStartHTOffset && aEndHTOffset) ||
                  (!mOffsetAcc && mOffsetAccIdx == -1 &&
                    !aStartHTOffset && !aEndHTOffset &&
                   mIncludeDefAttrs && aAttributes)),
                  "Wrong usage of TextAttrsMgr!");

  // Embedded objects are combined into own range with empty attributes set.
  if (mOffsetAcc && nsAccUtils::IsEmbeddedObject(mOffsetAcc)) {
    for (int32_t childIdx = mOffsetAccIdx - 1; childIdx >= 0; childIdx--) {
      Accessible* currAcc = mHyperTextAcc->GetChildAt(childIdx);
      if (!nsAccUtils::IsEmbeddedObject(currAcc))
        break;

      (*aStartHTOffset)--;
    }

    uint32_t childCount = mHyperTextAcc->ChildCount();
    for (uint32_t childIdx = mOffsetAccIdx + 1; childIdx < childCount;
         childIdx++) {
      Accessible* currAcc = mHyperTextAcc->GetChildAt(childIdx);
      if (!nsAccUtils::IsEmbeddedObject(currAcc))
        break;

      (*aEndHTOffset)++;
    }

    return;
  }

  // Get the content and frame of the accessible. In the case of document
  // accessible it's role content and root frame.
  nsIContent *hyperTextElm = mHyperTextAcc->GetContent();
  nsIFrame *rootFrame = mHyperTextAcc->GetFrame();
  NS_ASSERTION(rootFrame, "No frame for accessible!");
  if (!rootFrame)
    return;

  nsIContent *offsetNode = nullptr, *offsetElm = nullptr;
  nsIFrame *frame = nullptr;
  if (mOffsetAcc) {
    offsetNode = mOffsetAcc->GetContent();
    offsetElm = nsCoreUtils::GetDOMElementFor(offsetNode);
    frame = offsetElm->GetPrimaryFrame();
  }

  // "language" text attribute
  LangTextAttr langTextAttr(mHyperTextAcc, hyperTextElm, offsetNode);

  // "aria-invalid" text attribute
  InvalidTextAttr invalidTextAttr(hyperTextElm, offsetNode);

  // "background-color" text attribute
  BGColorTextAttr bgColorTextAttr(rootFrame, frame);

  // "color" text attribute
  ColorTextAttr colorTextAttr(rootFrame, frame);

  // "font-family" text attribute
  FontFamilyTextAttr fontFamilyTextAttr(rootFrame, frame);

  // "font-size" text attribute
  FontSizeTextAttr fontSizeTextAttr(rootFrame, frame);

  // "font-style" text attribute
  FontStyleTextAttr fontStyleTextAttr(rootFrame, frame);

  // "font-weight" text attribute
  FontWeightTextAttr fontWeightTextAttr(rootFrame, frame);

  // "auto-generated" text attribute
  AutoGeneratedTextAttr autoGenTextAttr(mHyperTextAcc, mOffsetAcc);

  // "text-underline(line-through)-style(color)" text attributes
  TextDecorTextAttr textDecorTextAttr(rootFrame, frame);

  // "text-position" text attribute
  TextPosTextAttr textPosTextAttr(rootFrame, frame);

  TextAttr* attrArray[] =
  {
    &langTextAttr,
    &invalidTextAttr,
    &bgColorTextAttr,
    &colorTextAttr,
    &fontFamilyTextAttr,
    &fontSizeTextAttr,
    &fontStyleTextAttr,
    &fontWeightTextAttr,
    &autoGenTextAttr,
    &textDecorTextAttr,
    &textPosTextAttr
  };

  // Expose text attributes if applicable.
  if (aAttributes) {
    for (uint32_t idx = 0; idx < ArrayLength(attrArray); idx++)
      attrArray[idx]->Expose(aAttributes, mIncludeDefAttrs);
  }

  // Expose text attributes range where they are applied if applicable.
  if (mOffsetAcc)
    GetRange(attrArray, ArrayLength(attrArray), aStartHTOffset, aEndHTOffset);
}

void
TextAttrsMgr::GetRange(TextAttr* aAttrArray[], uint32_t aAttrArrayLen,
                       int32_t* aStartHTOffset, int32_t* aEndHTOffset)
{
  // Navigate backward from anchor accessible to find start offset.
  for (int32_t childIdx = mOffsetAccIdx - 1; childIdx >= 0; childIdx--) {
    Accessible* currAcc = mHyperTextAcc->GetChildAt(childIdx);

    // Stop on embedded accessible since embedded accessibles are combined into
    // own range.
    if (nsAccUtils::IsEmbeddedObject(currAcc))
      break;

    bool offsetFound = false;
    for (uint32_t attrIdx = 0; attrIdx < aAttrArrayLen; attrIdx++) {
      TextAttr* textAttr = aAttrArray[attrIdx];
      if (!textAttr->Equal(currAcc)) {
        offsetFound = true;
        break;
      }
    }

    if (offsetFound)
      break;

    *(aStartHTOffset) -= nsAccUtils::TextLength(currAcc);
  }

  // Navigate forward from anchor accessible to find end offset.
  uint32_t childLen = mHyperTextAcc->ChildCount();
  for (uint32_t childIdx = mOffsetAccIdx + 1; childIdx < childLen; childIdx++) {
    Accessible* currAcc = mHyperTextAcc->GetChildAt(childIdx);
    if (nsAccUtils::IsEmbeddedObject(currAcc))
      break;

    bool offsetFound = false;
    for (uint32_t attrIdx = 0; attrIdx < aAttrArrayLen; attrIdx++) {
      TextAttr* textAttr = aAttrArray[attrIdx];

      // Alter the end offset when text attribute changes its value and stop
      // the search.
      if (!textAttr->Equal(currAcc)) {
        offsetFound = true;
        break;
      }
    }

    if (offsetFound)
      break;

    (*aEndHTOffset) += nsAccUtils::TextLength(currAcc);
  }
}


////////////////////////////////////////////////////////////////////////////////
// LangTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::LangTextAttr::
  LangTextAttr(HyperTextAccessible* aRoot,
               nsIContent* aRootElm, nsIContent* aElm) :
  TTextAttr<nsString>(!aElm), mRootContent(aRootElm)
{
  aRoot->Language(mRootNativeValue);
  mIsRootDefined =  !mRootNativeValue.IsEmpty();

  if (aElm) {
    nsCoreUtils::GetLanguageFor(aElm, mRootContent, mNativeValue);
    mIsDefined = !mNativeValue.IsEmpty();
  }
}

TextAttrsMgr::LangTextAttr::
  ~LangTextAttr() {}

bool
TextAttrsMgr::LangTextAttr::
  GetValueFor(Accessible* aAccessible, nsString* aValue)
{
  nsCoreUtils::GetLanguageFor(aAccessible->GetContent(), mRootContent, *aValue);
  return !aValue->IsEmpty();
}

void
TextAttrsMgr::LangTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nsString& aValue)
{
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::language, aValue);
}

////////////////////////////////////////////////////////////////////////////////
// InvalidTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::InvalidTextAttr::
  InvalidTextAttr(nsIContent* aRootElm, nsIContent* aElm) :
  TTextAttr<uint32_t>(!aElm), mRootElm(aRootElm)
{
  mIsRootDefined = GetValue(mRootElm, &mRootNativeValue);
  if (aElm)
    mIsDefined = GetValue(aElm, &mNativeValue);
}

bool
TextAttrsMgr::InvalidTextAttr::
  GetValueFor(Accessible* aAccessible, uint32_t* aValue)
{
  nsIContent* elm = nsCoreUtils::GetDOMElementFor(aAccessible->GetContent());
  return elm ? GetValue(elm, aValue) : false;
}

void
TextAttrsMgr::InvalidTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const uint32_t& aValue)
{
  switch (aValue) {
    case eFalse:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::invalid,
                             NS_LITERAL_STRING("false"));
      break;

    case eGrammar:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::invalid,
                             NS_LITERAL_STRING("grammar"));
      break;

    case eSpelling:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::invalid,
                             NS_LITERAL_STRING("spelling"));
      break;

    case eTrue:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::invalid,
                             NS_LITERAL_STRING("true"));
      break;
  }
}

bool
TextAttrsMgr::InvalidTextAttr::
  GetValue(nsIContent* aElm, uint32_t* aValue)
{
  nsIContent* elm = aElm;
  do {
    if (nsAccUtils::HasDefinedARIAToken(elm, nsGkAtoms::aria_invalid)) {
      static nsIContent::AttrValuesArray tokens[] =
        { &nsGkAtoms::_false, &nsGkAtoms::grammar, &nsGkAtoms::spelling,
          nullptr };

      int32_t idx = elm->FindAttrValueIn(kNameSpaceID_None,
                                         nsGkAtoms::aria_invalid, tokens,
                                         eCaseMatters);
      switch (idx) {
        case 0:
          *aValue = eFalse;
          return true;
        case 1:
          *aValue = eGrammar;
          return true;
        case 2:
          *aValue = eSpelling;
          return true;
        default:
          *aValue = eTrue;
          return true;
      }
    }
  } while ((elm = elm->GetParent()) && elm != mRootElm);

  return false;
}


////////////////////////////////////////////////////////////////////////////////
// BGColorTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::BGColorTextAttr::
  BGColorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nscolor>(!aFrame), mRootFrame(aRootFrame)
{
  mIsRootDefined = GetColor(mRootFrame, &mRootNativeValue);
  if (aFrame)
    mIsDefined = GetColor(aFrame, &mNativeValue);
}

bool
TextAttrsMgr::BGColorTextAttr::
  GetValueFor(Accessible* aAccessible, nscolor* aValue)
{
  nsIContent* elm = nsCoreUtils::GetDOMElementFor(aAccessible->GetContent());
  nsIFrame* frame = elm->GetPrimaryFrame();
  return frame ? GetColor(frame, aValue) : false;
}

void
TextAttrsMgr::BGColorTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nscolor& aValue)
{
  nsAutoString formattedValue;
  StyleInfo::FormatColor(aValue, formattedValue);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::backgroundColor,
                         formattedValue);
}

bool
TextAttrsMgr::BGColorTextAttr::
  GetColor(nsIFrame* aFrame, nscolor* aColor)
{
  const nsStyleBackground* styleBackground = aFrame->StyleBackground();

  if (NS_GET_A(styleBackground->mBackgroundColor) > 0) {
    *aColor = styleBackground->mBackgroundColor;
    return true;
  }

  nsIFrame *parentFrame = aFrame->GetParent();
  if (!parentFrame) {
    *aColor = aFrame->PresContext()->DefaultBackgroundColor();
    return true;
  }

  // Each frame of parents chain for the initially passed 'aFrame' has
  // transparent background color. So background color isn't changed from
  // 'mRootFrame' to initially passed 'aFrame'.
  if (parentFrame == mRootFrame)
    return false;

  return GetColor(parentFrame, aColor);
}


////////////////////////////////////////////////////////////////////////////////
// ColorTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::ColorTextAttr::
  ColorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nscolor>(!aFrame)
{
  mRootNativeValue = aRootFrame->StyleColor()->mColor;
  mIsRootDefined = true;

  if (aFrame) {
    mNativeValue = aFrame->StyleColor()->mColor;
    mIsDefined = true;
  }
}

bool
TextAttrsMgr::ColorTextAttr::
  GetValueFor(Accessible* aAccessible, nscolor* aValue)
{
  nsIContent* elm = nsCoreUtils::GetDOMElementFor(aAccessible->GetContent());
  nsIFrame* frame = elm->GetPrimaryFrame();
  if (frame) {
    *aValue = frame->StyleColor()->mColor;
    return true;
  }

  return false;
}

void
TextAttrsMgr::ColorTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nscolor& aValue)
{
  nsAutoString formattedValue;
  StyleInfo::FormatColor(aValue, formattedValue);
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::color, formattedValue);
}


////////////////////////////////////////////////////////////////////////////////
// FontFamilyTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::FontFamilyTextAttr::
  FontFamilyTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nsString>(!aFrame)
{
  mIsRootDefined = GetFontFamily(aRootFrame, mRootNativeValue);

  if (aFrame)
    mIsDefined = GetFontFamily(aFrame, mNativeValue);
}

bool
TextAttrsMgr::FontFamilyTextAttr::
  GetValueFor(Accessible* aAccessible, nsString* aValue)
{
  nsIContent* elm = nsCoreUtils::GetDOMElementFor(aAccessible->GetContent());
  nsIFrame* frame = elm->GetPrimaryFrame();
  return frame ? GetFontFamily(frame, *aValue) : false;
}

void
TextAttrsMgr::FontFamilyTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nsString& aValue)
{
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::font_family, aValue);
}

bool
TextAttrsMgr::FontFamilyTextAttr::
  GetFontFamily(nsIFrame* aFrame, nsString& aFamily)
{
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm));

  gfxFontGroup* fontGroup = fm->GetThebesFontGroup();
  gfxFont* font = fontGroup->GetFontAt(0);
  gfxFontEntry* fontEntry = font->GetFontEntry();
  aFamily = fontEntry->FamilyName();
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// FontSizeTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::FontSizeTextAttr::
  FontSizeTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nscoord>(!aFrame)
{
  mDC = aRootFrame->PresContext()->DeviceContext();

  mRootNativeValue = aRootFrame->StyleFont()->mSize;
  mIsRootDefined = true;

  if (aFrame) {
    mNativeValue = aFrame->StyleFont()->mSize;
    mIsDefined = true;
  }
}

bool
TextAttrsMgr::FontSizeTextAttr::
  GetValueFor(Accessible* aAccessible, nscoord* aValue)
{
  nsIContent* content = nsCoreUtils::GetDOMElementFor(aAccessible->GetContent());
  nsIFrame* frame = content->GetPrimaryFrame();
  if (frame) {
    *aValue = frame->StyleFont()->mSize;
    return true;
  }

  return false;
}

void
TextAttrsMgr::FontSizeTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nscoord& aValue)
{
  // Convert from nscoord to pt.
  //
  // Note: according to IA2, "The conversion doesn't have to be exact.
  // The intent is to give the user a feel for the size of the text."
  //
  // ATK does not specify a unit and will likely follow IA2 here.
  //
  // XXX todo: consider sharing this code with layout module? (bug 474621)
  float px =
    NSAppUnitsToFloatPixels(aValue, mozilla::AppUnitsPerCSSPixel());
  // Each pt is 4/3 of a CSS pixel.
  int pts = NS_lround(px*3/4);

  nsAutoString value;
  value.AppendInt(pts);
  value.Append(NS_LITERAL_STRING("pt"));

  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::font_size, value);
}


////////////////////////////////////////////////////////////////////////////////
// FontStyleTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::FontStyleTextAttr::
  FontStyleTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<nscoord>(!aFrame)
{
  mRootNativeValue = aRootFrame->StyleFont()->mFont.style;
  mIsRootDefined = true;

  if (aFrame) {
    mNativeValue = aFrame->StyleFont()->mFont.style;
    mIsDefined = true;
  }
}

bool
TextAttrsMgr::FontStyleTextAttr::
  GetValueFor(Accessible* aAccessible, nscoord* aValue)
{
  nsIContent* elm = nsCoreUtils::GetDOMElementFor(aAccessible->GetContent());
  nsIFrame* frame = elm->GetPrimaryFrame();
  if (frame) {
    *aValue = frame->StyleFont()->mFont.style;
    return true;
  }

  return false;
}

void
TextAttrsMgr::FontStyleTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const nscoord& aValue)
{
  nsAutoString formattedValue;
  StyleInfo::FormatFontStyle(aValue, formattedValue);

  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::font_style, formattedValue);
}


////////////////////////////////////////////////////////////////////////////////
// FontWeightTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::FontWeightTextAttr::
  FontWeightTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<int32_t>(!aFrame)
{
  mRootNativeValue = GetFontWeight(aRootFrame);
  mIsRootDefined = true;

  if (aFrame) {
    mNativeValue = GetFontWeight(aFrame);
    mIsDefined = true;
  }
}

bool
TextAttrsMgr::FontWeightTextAttr::
  GetValueFor(Accessible* aAccessible, int32_t* aValue)
{
  nsIContent* elm = nsCoreUtils::GetDOMElementFor(aAccessible->GetContent());
  nsIFrame* frame = elm->GetPrimaryFrame();
  if (frame) {
    *aValue = GetFontWeight(frame);
    return true;
  }

  return false;
}

void
TextAttrsMgr::FontWeightTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const int32_t& aValue)
{
  nsAutoString formattedValue;
  formattedValue.AppendInt(aValue);

  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::fontWeight, formattedValue);
}

int32_t
TextAttrsMgr::FontWeightTextAttr::
  GetFontWeight(nsIFrame* aFrame)
{
  // nsFont::width isn't suitable here because it's necessary to expose real
  // value of font weight (used font might not have some font weight values).
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm));

  gfxFontGroup *fontGroup = fm->GetThebesFontGroup();
  gfxFont *font = fontGroup->GetFontAt(0);

  // When there doesn't exist a bold font in the family and so the rendering of
  // a non-bold font face is changed so that the user sees what looks like a
  // bold font, i.e. synthetic bolding is used. IsSyntheticBold method is only
  // needed on Mac, but it is "safe" to use on all platforms.  (For non-Mac
  // platforms it always return false.)
  if (font->IsSyntheticBold())
    return 700;

#ifdef MOZ_PANGO
  // On Linux, font->GetStyle()->weight will give the absolute weight requested
  // of the font face. The Linux code uses the gfxFontEntry constructor which
  // doesn't initialize the weight field.
  return font->GetStyle()->weight;
#else
  // On Windows, font->GetStyle()->weight will give the same weight as
  // fontEntry->Weight(), the weight of the first font in the font group, which
  // may not be the weight of the font face used to render the characters.
  // On Mac, font->GetStyle()->weight will just give the same number as
  // getComputedStyle(). fontEntry->Weight() will give the weight of the font
  // face used.
  gfxFontEntry *fontEntry = font->GetFontEntry();
  return fontEntry->Weight();
#endif
}

////////////////////////////////////////////////////////////////////////////////
// AutoGeneratedTextAttr
////////////////////////////////////////////////////////////////////////////////
TextAttrsMgr::AutoGeneratedTextAttr::
  AutoGeneratedTextAttr(HyperTextAccessible* aHyperTextAcc,
                        Accessible* aAccessible) :
  TTextAttr<bool>(!aAccessible)
{
  mRootNativeValue = false;
  mIsRootDefined = false;

  if (aAccessible)
    mIsDefined = mNativeValue = (aAccessible->NativeRole() == roles::STATICTEXT);
}

bool
TextAttrsMgr::AutoGeneratedTextAttr::
  GetValueFor(Accessible* aAccessible, bool* aValue)
{
  return *aValue = (aAccessible->NativeRole() == roles::STATICTEXT);
}

void
TextAttrsMgr::AutoGeneratedTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const bool& aValue)
{
  nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::auto_generated,
                         aValue ? NS_LITERAL_STRING("true") : NS_LITERAL_STRING("false"));
}


////////////////////////////////////////////////////////////////////////////////
// TextDecorTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::TextDecorValue::
  TextDecorValue(nsIFrame* aFrame)
{
  const nsStyleTextReset* textReset = aFrame->StyleTextReset();
  mStyle = textReset->GetDecorationStyle();

  bool isForegroundColor = false;
  textReset->GetDecorationColor(mColor, isForegroundColor);
  if (isForegroundColor)
    mColor = aFrame->StyleColor()->mColor;

  mLine = textReset->mTextDecorationLine &
    (NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE |
     NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH);
}

TextAttrsMgr::TextDecorTextAttr::
  TextDecorTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<TextDecorValue>(!aFrame)
{
  mRootNativeValue = TextDecorValue(aRootFrame);
  mIsRootDefined = mRootNativeValue.IsDefined();

  if (aFrame) {
    mNativeValue = TextDecorValue(aFrame);
    mIsDefined = mNativeValue.IsDefined();
  }
}

bool
TextAttrsMgr::TextDecorTextAttr::
  GetValueFor(Accessible* aAccessible, TextDecorValue* aValue)
{
  nsIContent* elm = nsCoreUtils::GetDOMElementFor(aAccessible->GetContent());
  nsIFrame* frame = elm->GetPrimaryFrame();
  if (frame) {
    *aValue = TextDecorValue(frame);
    return aValue->IsDefined();
  }

  return false;
}

void
TextAttrsMgr::TextDecorTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const TextDecorValue& aValue)
{
  if (aValue.IsUnderline()) {
    nsAutoString formattedStyle;
    StyleInfo::FormatTextDecorationStyle(aValue.Style(), formattedStyle);
    nsAccUtils::SetAccAttr(aAttributes,
                           nsGkAtoms::textUnderlineStyle,
                           formattedStyle);

    nsAutoString formattedColor;
    StyleInfo::FormatColor(aValue.Color(), formattedColor);
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textUnderlineColor,
                           formattedColor);
    return;
  }

  if (aValue.IsLineThrough()) {
    nsAutoString formattedStyle;
    StyleInfo::FormatTextDecorationStyle(aValue.Style(), formattedStyle);
    nsAccUtils::SetAccAttr(aAttributes,
                           nsGkAtoms::textLineThroughStyle,
                           formattedStyle);

    nsAutoString formattedColor;
    StyleInfo::FormatColor(aValue.Color(), formattedColor);
    nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textLineThroughColor,
                           formattedColor);
  }
}

////////////////////////////////////////////////////////////////////////////////
// TextPosTextAttr
////////////////////////////////////////////////////////////////////////////////

TextAttrsMgr::TextPosTextAttr::
  TextPosTextAttr(nsIFrame* aRootFrame, nsIFrame* aFrame) :
  TTextAttr<TextPosValue>(!aFrame)
{
  mRootNativeValue = GetTextPosValue(aRootFrame);
  mIsRootDefined = mRootNativeValue != eTextPosNone;

  if (aFrame) {
    mNativeValue = GetTextPosValue(aFrame);
    mIsDefined = mNativeValue != eTextPosNone;
  }
}

bool
TextAttrsMgr::TextPosTextAttr::
  GetValueFor(Accessible* aAccessible, TextPosValue* aValue)
{
  nsIContent* elm = nsCoreUtils::GetDOMElementFor(aAccessible->GetContent());
  nsIFrame* frame = elm->GetPrimaryFrame();
  if (frame) {
    *aValue = GetTextPosValue(frame);
    return *aValue != eTextPosNone;
  }

  return false;
}

void
TextAttrsMgr::TextPosTextAttr::
  ExposeValue(nsIPersistentProperties* aAttributes, const TextPosValue& aValue)
{
  switch (aValue) {
    case eTextPosBaseline:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textPosition,
                             NS_LITERAL_STRING("baseline"));
      break;

    case eTextPosSub:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textPosition,
                             NS_LITERAL_STRING("sub"));
      break;

    case eTextPosSuper:
      nsAccUtils::SetAccAttr(aAttributes, nsGkAtoms::textPosition,
                             NS_LITERAL_STRING("super"));
      break;

    case eTextPosNone:
      break;
  }
}

TextAttrsMgr::TextPosValue
TextAttrsMgr::TextPosTextAttr::
  GetTextPosValue(nsIFrame* aFrame) const
{
  const nsStyleCoord& styleCoord = aFrame->StyleTextReset()->mVerticalAlign;
  switch (styleCoord.GetUnit()) {
    case eStyleUnit_Enumerated:
      switch (styleCoord.GetIntValue()) {
        case NS_STYLE_VERTICAL_ALIGN_BASELINE:
          return eTextPosBaseline;
        case NS_STYLE_VERTICAL_ALIGN_SUB:
          return eTextPosSub;
        case NS_STYLE_VERTICAL_ALIGN_SUPER:
          return eTextPosSuper;

        // No good guess for these:
        //   NS_STYLE_VERTICAL_ALIGN_TOP
        //   NS_STYLE_VERTICAL_ALIGN_TEXT_TOP
        //   NS_STYLE_VERTICAL_ALIGN_MIDDLE
        //   NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM
        //   NS_STYLE_VERTICAL_ALIGN_BOTTOM
        //   NS_STYLE_VERTICAL_ALIGN_MIDDLE_WITH_BASELINE
        // Do not expose value of text-position attribute.

        default:
          break;
      }
      return eTextPosNone;

    case eStyleUnit_Percent:
    {
      float percentValue = styleCoord.GetPercentValue();
      return percentValue > 0 ?
        eTextPosSuper :
        (percentValue < 0 ? eTextPosSub : eTextPosBaseline);
    }

    case eStyleUnit_Coord:
    {
       nscoord coordValue = styleCoord.GetCoordValue();
       return coordValue > 0 ?
         eTextPosSuper :
         (coordValue < 0 ? eTextPosSub : eTextPosBaseline);
    }

    case eStyleUnit_Null:
    case eStyleUnit_Normal:
    case eStyleUnit_Auto:
    case eStyleUnit_None:
    case eStyleUnit_Factor:
    case eStyleUnit_Degree:
    case eStyleUnit_Grad:
    case eStyleUnit_Radian:
    case eStyleUnit_Turn:
    case eStyleUnit_Integer:
    case eStyleUnit_Calc:
      break;
  }

  const nsIContent* content = aFrame->GetContent();
  if (content && content->IsHTML()) {
    const nsIAtom* tagName = content->Tag();
    if (tagName == nsGkAtoms::sup) 
      return eTextPosSuper;
    if (tagName == nsGkAtoms::sub) 
      return eTextPosSub;
  }

  return eTextPosNone;
}
