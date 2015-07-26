/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2007 David Smith (catfish.man@gmail.com)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012, 2013 Apple Inc. All rights reserved.
 *           (C) 2007 Eric Seidel (eric@webkit.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "sky/engine/core/dom/Element.h"

#include "gen/sky/core/CSSValueKeywords.h"
#include "gen/sky/core/HTMLNames.h"
#include "gen/sky/platform/RuntimeEnabledFeatures.h"
#include "sky/engine/bindings/exception_messages.h"
#include "sky/engine/bindings/exception_state.h"
#include "sky/engine/core/css/CSSStyleSheet.h"
#include "sky/engine/core/css/CSSValuePool.h"
#include "sky/engine/core/css/PropertySetCSSStyleDeclaration.h"
#include "sky/engine/core/css/StylePropertySet.h"
#include "sky/engine/core/css/parser/BisonCSSParser.h"
#include "sky/engine/core/css/resolver/StyleResolver.h"
#include "sky/engine/core/dom/Attr.h"
#include "sky/engine/core/dom/ClientRect.h"
#include "sky/engine/core/dom/ClientRectList.h"
#include "sky/engine/core/dom/DOMTokenList.h"
#include "sky/engine/core/dom/Document.h"
#include "sky/engine/core/dom/ElementDataCache.h"
#include "sky/engine/core/dom/ElementRareData.h"
#include "sky/engine/core/dom/ElementTraversal.h"
#include "sky/engine/core/dom/ExceptionCode.h"
#include "sky/engine/core/dom/Microtask.h"
#include "sky/engine/core/dom/MutationObserverInterestGroup.h"
#include "sky/engine/core/dom/MutationRecord.h"
#include "sky/engine/core/dom/NodeRenderStyle.h"
#include "sky/engine/core/dom/RenderTreeBuilder.h"
#include "sky/engine/core/dom/SelectorQuery.h"
#include "sky/engine/core/dom/StyleEngine.h"
#include "sky/engine/core/dom/Text.h"
#include "sky/engine/core/editing/TextIterator.h"
#include "sky/engine/core/editing/htmlediting.h"
#include "sky/engine/core/frame/FrameView.h"
#include "sky/engine/core/frame/LocalFrame.h"
#include "sky/engine/core/frame/Settings.h"
#include "sky/engine/core/html/HTMLElement.h"
#include "sky/engine/core/html/parser/HTMLParserIdioms.h"
#include "sky/engine/core/page/ChromeClient.h"
#include "sky/engine/core/page/Page.h"
#include "sky/engine/core/painting/Canvas.h"
#include "sky/engine/core/painting/PaintingCallback.h"
#include "sky/engine/core/painting/PaintingTasks.h"
#include "sky/engine/core/painting/PictureRecorder.h"
#include "sky/engine/core/rendering/RenderLayer.h"
#include "sky/engine/core/rendering/RenderView.h"
#include "sky/engine/platform/EventDispatchForbiddenScope.h"
#include "sky/engine/tonic/dart_state.h"
#include "sky/engine/wtf/BitVector.h"
#include "sky/engine/wtf/HashFunctions.h"
#include "sky/engine/wtf/text/CString.h"
#include "sky/engine/wtf/text/StringBuilder.h"
#include "sky/engine/wtf/text/TextPosition.h"

namespace blink {

PassRefPtr<Element> Element::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new Element(tagName, document, CreateElement));
}

Element::Element(const QualifiedName& tagName, Document* document, ConstructionType type)
    : ContainerNode(document, type)
    , m_tagName(tagName)
{
}

Element::~Element()
{
    ASSERT(needsAttach());
}

inline ElementRareData* Element::elementRareData() const
{
    ASSERT(hasRareData());
    return static_cast<ElementRareData*>(rareData());
}

inline ElementRareData& Element::ensureElementRareData()
{
    return static_cast<ElementRareData&>(ensureRareData());
}

PassRefPtr<Node> Element::cloneNode(bool deep)
{
    return deep ? cloneElementWithChildren() : cloneElementWithoutChildren();
}

PassRefPtr<Element> Element::cloneElementWithChildren()
{
    RefPtr<Element> clone = cloneElementWithoutChildren();
    cloneChildNodes(clone.get());
    return clone.release();
}

PassRefPtr<Element> Element::cloneElementWithoutChildren()
{
    RefPtr<Element> clone = cloneElementWithoutAttributesAndChildren();
    clone->cloneDataFromElement(*this);
    return clone.release();
}

PassRefPtr<Element> Element::cloneElementWithoutAttributesAndChildren()
{
    return document().createElement(tagQName(), false);
}

void Element::removeAttribute(const QualifiedName& name)
{
    if (!elementData())
        return;

    size_t index = elementData()->attributes().findIndex(name);
    if (index == kNotFound)
        return;

    removeAttributeInternal(index, NotInSynchronizationOfLazyAttribute);
}

void Element::setBooleanAttribute(const QualifiedName& name, bool value)
{
    if (value)
        setAttribute(name, emptyAtom);
    else
        removeAttribute(name);
}

Node::NodeType Element::nodeType() const
{
    return ELEMENT_NODE;
}

void Element::synchronizeAllAttributes() const
{
    synchronizeAttribute(HTMLNames::styleAttr.localName());
}

String Element::contentEditable() const
{
    const AtomicString& value = getAttribute(HTMLNames::contenteditableAttr);

    if (value.isNull())
        return "inherit";
    if (value.isEmpty() || equalIgnoringCase(value, "true"))
        return "true";
    if (equalIgnoringCase(value, "false"))
         return "false";
    if (equalIgnoringCase(value, "plaintext-only"))
        return "plaintext-only";

    return "inherit";
}

void Element::setContentEditable(const String& enabled, ExceptionState& exceptionState)
{
    if (equalIgnoringCase(enabled, "true"))
        setAttribute(HTMLNames::contenteditableAttr, "true");
    else if (equalIgnoringCase(enabled, "false"))
        setAttribute(HTMLNames::contenteditableAttr, "false");
    else if (equalIgnoringCase(enabled, "plaintext-only"))
        setAttribute(HTMLNames::contenteditableAttr, "plaintext-only");
    else if (equalIgnoringCase(enabled, "inherit"))
        removeAttribute(HTMLNames::contenteditableAttr);
    else
        exceptionState.ThrowDOMException(SyntaxError, "The value provided ('" + enabled + "') is not one of 'true', 'false', 'plaintext-only', or 'inherit'.");
}

bool Element::spellcheck() const
{
    return isSpellCheckingEnabled();
}

void Element::setSpellcheck(bool enable)
{
    setAttribute(HTMLNames::spellcheckAttr, enable ? "true" : "false");
}

// Returns the conforming 'dir' value associated with the state the attribute is in (in its canonical case), if any,
// or the empty string if the attribute is in a state that has no associated keyword value or if the attribute is
// not in a defined state (e.g. the attribute is missing and there is no missing value default).
// http://www.whatwg.org/specs/web-apps/current-work/multipage/common-dom-interfaces.html#limited-to-only-known-values
static inline const AtomicString& toValidDirValue(const AtomicString& value)
{
    DEFINE_STATIC_LOCAL(const AtomicString, ltrValue, ("ltr", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, rtlValue, ("rtl", AtomicString::ConstructFromLiteral));
    DEFINE_STATIC_LOCAL(const AtomicString, autoValue, ("auto", AtomicString::ConstructFromLiteral));

    if (equalIgnoringCase(value, ltrValue))
        return ltrValue;
    if (equalIgnoringCase(value, rtlValue))
        return rtlValue;
    if (equalIgnoringCase(value, autoValue))
        return autoValue;
    return nullAtom;
}

const AtomicString& Element::dir()
{
    return toValidDirValue(getAttribute(HTMLNames::dirAttr));
}

void Element::setDir(const AtomicString& value)
{
    setAttribute(HTMLNames::dirAttr, value);
}

int Element::offsetLeft()
{
    document().updateLayout();
    if (RenderBoxModelObject* renderer = renderBoxModelObject())
        return renderer->offsetLeft();
    return 0;
}

int Element::offsetTop()
{
    document().updateLayout();
    if (RenderBoxModelObject* renderer = renderBoxModelObject())
        return renderer->pixelSnappedOffsetTop();
    return 0;
}

int Element::offsetWidth()
{
    document().updateLayout();
    if (RenderBoxModelObject* renderer = renderBoxModelObject())
        return renderer->pixelSnappedOffsetWidth();
    return 0;
}

int Element::offsetHeight()
{
    document().updateLayout();
    if (RenderBoxModelObject* renderer = renderBoxModelObject())
        return renderer->pixelSnappedOffsetHeight();
    return 0;
}

Element* Element::offsetParent()
{
    document().updateLayout();
    if (RenderObject* renderer = this->renderer())
        return renderer->offsetParent();
    return 0;
}

int Element::clientLeft()
{
    document().updateLayout();

    if (RenderBox* renderer = renderBox())
        return roundToInt(renderer->clientLeft());
    return 0;
}

int Element::clientTop()
{
    document().updateLayout();

    if (RenderBox* renderer = renderBox())
        return roundToInt(renderer->clientTop());
    return 0;
}

int Element::clientWidth()
{
    document().updateLayout();

    if (RenderBox* renderer = renderBox())
        return renderer->pixelSnappedClientWidth();
    return 0;
}

int Element::clientHeight()
{
    document().updateLayout();

    if (RenderBox* renderer = renderBox())
        return renderer->pixelSnappedClientHeight();
    return 0;
}

PassRefPtr<ClientRectList> Element::getClientRects()
{
    document().updateLayout();

    RenderBoxModelObject* renderBoxModelObject = this->renderBoxModelObject();
    if (!renderBoxModelObject)
        return ClientRectList::create();

    // FIXME: Handle SVG elements.
    // FIXME: Handle table/inline-table with a caption.

    Vector<FloatQuad> quads;
    renderBoxModelObject->absoluteQuads(quads);
    return ClientRectList::create(quads);
}

PassRefPtr<ClientRect> Element::getBoundingClientRect()
{
    document().updateLayout();

    Vector<FloatQuad> quads;
    // Get the bounding rectangle from the box model.
    if (renderBoxModelObject())
        renderBoxModelObject()->absoluteQuads(quads);

    if (quads.isEmpty())
        return ClientRect::create();

    FloatRect result = quads[0].boundingBox();
    for (size_t i = 1; i < quads.size(); ++i)
        result.unite(quads[i].boundingBox());

    return ClientRect::create(result);
}

void Element::requestPaint(PassOwnPtr<PaintingCallback> callback)
{
    PaintingTasks::enqueueRequest(this, callback);
    document().scheduleVisualUpdate();
}

void Element::setAttribute(const AtomicString& localName, const AtomicString& value, ExceptionState& exceptionState)
{
    if (!Document::isValidName(localName)) {
        exceptionState.ThrowDOMException(InvalidCharacterError, "'" + localName + "' is not a valid attribute name.");
        return;
    }

    synchronizeAttribute(localName);

    if (!elementData()) {
        setAttributeInternal(kNotFound, QualifiedName(localName), value, NotInSynchronizationOfLazyAttribute);
        return;
    }

    AttributeCollection attributes = elementData()->attributes();
    size_t index = attributes.findIndex(localName);
    const QualifiedName& qName = index != kNotFound ? attributes[index].name() : QualifiedName(localName);
    setAttributeInternal(index, qName, value, NotInSynchronizationOfLazyAttribute);
}

void Element::setAttribute(const QualifiedName& name, const AtomicString& value)
{
    synchronizeAttribute(name.localName());
    size_t index = elementData() ? elementData()->attributes().findIndex(name) : kNotFound;
    setAttributeInternal(index, name, value, NotInSynchronizationOfLazyAttribute);
}

void Element::setSynchronizedLazyAttribute(const QualifiedName& name, const AtomicString& value)
{
    size_t index = elementData() ? elementData()->attributes().findIndex(name) : kNotFound;
    setAttributeInternal(index, name, value, InSynchronizationOfLazyAttribute);
}

ALWAYS_INLINE void Element::setAttributeInternal(size_t index, const QualifiedName& name, const AtomicString& newValue, SynchronizationOfLazyAttribute inSynchronizationOfLazyAttribute)
{
    if (newValue.isNull()) {
        if (index != kNotFound)
            removeAttributeInternal(index, inSynchronizationOfLazyAttribute);
        return;
    }

    if (index == kNotFound) {
        appendAttributeInternal(name, newValue, inSynchronizationOfLazyAttribute);
        return;
    }

    const Attribute& existingAttribute = elementData()->attributes().at(index);
    QualifiedName existingAttributeName = existingAttribute.name();

    if (newValue == existingAttribute.value())
        return;

    if (!inSynchronizationOfLazyAttribute)
        willModifyAttribute(existingAttributeName, existingAttribute.value(), newValue);

    ensureUniqueElementData().attributes().at(index).setValue(newValue);

    if (!inSynchronizationOfLazyAttribute)
        attributeChanged(existingAttributeName, newValue);
}

void Element::attributeChanged(const QualifiedName& name, const AtomicString& newValue, AttributeModificationReason reason)
{
    bool testShouldInvalidateStyle = inActiveDocument() && styleChangeType() < SubtreeStyleChange;

    if (isStyledElement() && name == HTMLNames::styleAttr) {
        styleAttributeChanged(newValue);
    }

    if (name == HTMLNames::idAttr) {
        AtomicString oldId = elementData()->idForStyleResolution();
        AtomicString newId = newValue;
        if (newId != oldId) {
            elementData()->setIdForStyleResolution(newId);
            if (testShouldInvalidateStyle && (affectedByIdSelector(oldId) || affectedByIdSelector(newId)))
                setNeedsStyleRecalc(LocalStyleChange);
        }
    } else if (name == HTMLNames::classAttr) {
        classAttributeChanged(newValue);
    }
}

inline void Element::attributeChangedFromParserOrByCloning(const QualifiedName& name, const AtomicString& newValue, AttributeModificationReason reason)
{
    attributeChanged(name, newValue, reason);
}

void Element::classAttributeChanged(const AtomicString& newClassString)
{
    bool testShouldInvalidateStyle = inActiveDocument() && styleChangeType() < SubtreeStyleChange;

    ASSERT(elementData());
    const SpaceSplitString oldClasses = elementData()->classNames();
    elementData()->setClass(newClassString, false);
    const SpaceSplitString& newClasses = elementData()->classNames();
    if (testShouldInvalidateStyle && classChangeNeedsStyleRecalc(oldClasses, newClasses))
        setNeedsStyleRecalc(LocalStyleChange);
    if (!newClasses.size())
        elementData()->clearClass();
}

bool Element::classChangeNeedsStyleRecalc(const SpaceSplitString& oldClasses, const SpaceSplitString& newClasses)
{
    // Class vectors tend to be very short. This is faster than using a hash table.
    BitVector remainingClassBits;
    remainingClassBits.ensureSize(oldClasses.size());

    for (unsigned i = 0; i < newClasses.size(); ++i) {
        bool found = false;
        for (unsigned j = 0; j < oldClasses.size(); ++j) {
            if (newClasses[i] == oldClasses[j]) {
                // Mark each class that is still in the newClasses so we can skip doing
                // an n^2 search below when looking for removals. We can't break from
                // this loop early since a class can appear more than once.
                remainingClassBits.quickSet(j);
                found = true;
            }
        }
        // Class was added.
        if (!found && affectedByClassSelector(newClasses[i]))
            return true;
    }

    for (unsigned i = 0; i < oldClasses.size(); ++i) {
        if (remainingClassBits.quickGet(i))
            continue;
        // Class was removed.
        if (affectedByClassSelector(oldClasses[i]))
            return true;
    }
    return false;
}

void Element::parserSetAttributes(const Vector<Attribute>& attributeVector)
{
    ASSERT(!inDocument());
    ASSERT(!parentNode());
    ASSERT(!m_elementData);

    if (attributeVector.isEmpty())
        return;

    if (document().elementDataCache())
        m_elementData = document().elementDataCache()->cachedShareableElementDataWithAttributes(attributeVector);
    else
        m_elementData = ShareableElementData::createWithAttributes(attributeVector);

    // Use attributeVector instead of m_elementData because attributeChanged might modify m_elementData.
    for (unsigned i = 0; i < attributeVector.size(); ++i)
        attributeChangedFromParserOrByCloning(attributeVector[i].name(), attributeVector[i].value(), ModifiedDirectly);
}

bool Element::hasEquivalentAttributes(const Element* other) const
{
    synchronizeAllAttributes();
    other->synchronizeAllAttributes();
    if (elementData() == other->elementData())
        return true;
    if (elementData())
        return elementData()->isEquivalent(other->elementData());
    if (other->elementData())
        return other->elementData()->isEquivalent(elementData());
    return true;
}

String Element::nodeName() const
{
    return m_tagName.localName();
}

const AtomicString Element::imageSourceURL() const
{
    return getAttribute(HTMLNames::srcAttr);
}

RenderObject* Element::createRenderer(RenderStyle* style)
{
    return RenderObject::createObject(this, style);
}

void Element::insertedInto(ContainerNode* insertionPoint)
{
    // need to do superclass processing first so inDocument() is true
    // by the time we reach updateId
    ContainerNode::insertedInto(insertionPoint);

    if (!insertionPoint->isInTreeScope())
        return;

    TreeScope& scope = insertionPoint->treeScope();
    if (scope != treeScope())
        return;

    const AtomicString& idValue = getIdAttribute();
    if (!idValue.isNull())
        updateId(scope, nullAtom, idValue);
}

void Element::removedFrom(ContainerNode* insertionPoint)
{
    if (insertionPoint->isInTreeScope() && treeScope() == document()) {
        const AtomicString& idValue = getIdAttribute();
        if (!idValue.isNull())
            updateId(insertionPoint->treeScope(), idValue, nullAtom);
    }

    ContainerNode::removedFrom(insertionPoint);
}

void Element::attach(const AttachContext& context)
{
    ASSERT(document().inStyleRecalc());

    // We've already been through detach when doing an attach, but we might
    // need to clear any state that's been added since then.
    if (hasRareData() && styleChangeType() == NeedsReattachStyleChange) {
        ElementRareData* data = elementRareData();
        data->clearComputedStyle();
    }

    RenderTreeBuilder(this, context.resolvedStyle).createRendererForElementIfNeeded();

    ContainerNode::attach(context);
}

void Element::detach(const AttachContext& context)
{
    if (hasRareData()) {
        ElementRareData* data = elementRareData();

        // attach() will perform the below steps for us when inside recalcStyle.
        if (!document().inStyleRecalc()) {
            data->clearComputedStyle();
        }

    }
    ContainerNode::detach(context);
}

PassRefPtr<RenderStyle> Element::styleForRenderer()
{
    ASSERT(document().inStyleRecalc());

    // FIXME: Instead of clearing updates that may have been added from calls to styleForElement
    // outside recalcStyle, we should just never set them if we're not inside recalcStyle.

    RefPtr<RenderStyle> style = document().styleResolver().styleForElement(this);
    ASSERT(style);

    document().didRecalculateStyleForElement();
    return style.release();
}

void Element::recalcStyle(StyleRecalcChange change)
{
    ASSERT(document().inStyleRecalc());
    ASSERT(!parentNode()->needsStyleRecalc());

    if (change >= Inherit || needsStyleRecalc()) {
        if (hasRareData()) {
            ElementRareData* data = elementRareData();
            data->clearComputedStyle();
        }
        if (parentRenderStyle())
            change = recalcOwnStyle(change);
        clearNeedsStyleRecalc();
    }

    // If we reattached we don't need to recalc the style of our descendants anymore.
    if ((change >= Inherit && change < Reattach) || childNeedsStyleRecalc()) {
        recalcChildStyle(change);
        clearChildNeedsStyleRecalc();
    }
}

StyleRecalcChange Element::recalcOwnStyle(StyleRecalcChange change)
{
    ASSERT(document().inStyleRecalc());
    ASSERT(!parentNode()->needsStyleRecalc());
    ASSERT(change >= Inherit || needsStyleRecalc());
    ASSERT(parentRenderStyle());

    RefPtr<RenderStyle> oldStyle = renderStyle();
    RefPtr<RenderStyle> newStyle = styleForRenderer();
    StyleRecalcChange localChange = RenderStyle::stylePropagationDiff(oldStyle.get(), newStyle.get());

    ASSERT(newStyle);

    if (localChange == Reattach) {
        AttachContext reattachContext;
        reattachContext.resolvedStyle = newStyle.get();
        reattach(reattachContext);
        return Reattach;
    }

    ASSERT(oldStyle);

    if (RenderObject* renderer = this->renderer()) {
        if (localChange != NoChange)
            renderer->setStyle(newStyle.get());
    }

    if (styleChangeType() >= SubtreeStyleChange)
        return Force;

    if (change > Inherit || localChange > Inherit)
        return max(localChange, change);

    return localChange;
}

void Element::recalcChildStyle(StyleRecalcChange change)
{
    ASSERT(document().inStyleRecalc());
    ASSERT(change >= Inherit || childNeedsStyleRecalc());
    ASSERT(!needsStyleRecalc());

    if (change > Inherit || childNeedsStyleRecalc()) {

        // This loop is deliberately backwards because we use insertBefore in the rendering tree, and want to avoid
        // a potentially n^2 loop to find the insertion point while resolving style. Having us start from the last
        // child and work our way back means in the common case, we'll find the insertion point in O(1) time.
        // See crbug.com/288225
        StyleResolver& styleResolver = document().styleResolver();
        for (Node* child = lastChild(); child; child = child->previousSibling()) {
            if (child->isTextNode()) {
                toText(child)->recalcTextStyle(change);
            } else if (child->isElementNode()) {
                Element* element = toElement(child);
                if (element->shouldCallRecalcStyle(change))
                    element->recalcStyle(change);
                else if (element->supportsStyleSharing())
                    styleResolver.addToStyleSharingList(*element);
            }
        }
    }
}

double Element::x() const
{
    if (RenderBox* box = renderBox())
        return box->x();
    return 0;
}

void Element::setX(double x)
{
    if (RenderBox* box = renderBox())
        return box->setX(x);
}

double Element::y() const
{
    if (RenderBox* box = renderBox())
        return box->y();
    return 0;
}

void Element::setY(double y)
{
    if (RenderBox* box = renderBox())
        return box->setY(y);
}

double Element::width() const
{
    if (RenderBox* box = renderBox())
        return box->width();
    return 0;
}

void Element::setWidth(double width)
{
    if (RenderBox* box = renderBox()) {
        box->setWidth(width);
        // TODO(ojan): Remove override widths once we remove box layout from the C++ code.
        box->setOverrideLogicalContentWidth(width);
    }
}

double Element::height() const
{
    if (RenderBox* box = renderBox())
        return box->height();
    return 0;
}

void Element::setHeight(double height)
{
    if (RenderBox* box = renderBox())
        return box->setHeight(height);
}

double Element::minContentWidth() const
{
    if (RenderBox* box = renderBox())
        return box->minPreferredLogicalWidth();
    return 0;
}

void Element::setMinContentWidth(double width)
{
    if (RenderBox* box = renderBox())
        return box->setMinPreferredLogicalWidth(width);
}

double Element::maxContentWidth() const
{
    if (RenderBox* box = renderBox())
        return box->maxPreferredLogicalWidth();
    return 0;
}

void Element::setMaxContentWidth(double width)
{
    if (RenderBox* box = renderBox())
        return box->setMaxPreferredLogicalWidth(width);
}

double Element::alphabeticBaseline() const
{
    if (RenderBox* box = renderBox())
        return box->firstLineBoxBaseline(FontBaselineOrAuto(AlphabeticBaseline));
    return 0;
}

double Element::ideographicBaseline() const
{
    if (RenderBox* box = renderBox())
        return box->firstLineBoxBaseline(FontBaselineOrAuto(IdeographicBaseline));
    return 0;
}

void Element::setNeedsLayout()
{
    if (RenderBox* box = renderBox())
        box->setNeedsLayout();
}

void Element::layout()
{
    if (RenderBox* box = renderBox())
        box->layoutIfNeeded();
}

void Element::childrenChanged(const ChildrenChange& change)
{
    ContainerNode::childrenChanged(change);
}

#ifndef NDEBUG
void Element::formatForDebugger(char* buffer, unsigned length) const
{
    StringBuilder result;
    String s;

    result.append(nodeName());

    s = getIdAttribute();
    if (s.length() > 0) {
        if (result.length() > 0)
            result.appendLiteral("; ");
        result.appendLiteral("id=");
        result.append(s);
    }

    s = getAttribute(HTMLNames::classAttr);
    if (s.length() > 0) {
        if (result.length() > 0)
            result.appendLiteral("; ");
        result.appendLiteral("class=");
        result.append(s);
    }

    strncpy(buffer, result.toString().utf8().data(), length - 1);
}
#endif

void Element::removeAttributeInternal(size_t index, SynchronizationOfLazyAttribute inSynchronizationOfLazyAttribute)
{
    MutableAttributeCollection attributes = ensureUniqueElementData().attributes();
    ASSERT_WITH_SECURITY_IMPLICATION(index < attributes.size());

    QualifiedName name = attributes[index].name();
    AtomicString valueBeingRemoved = attributes[index].value();

    if (!inSynchronizationOfLazyAttribute) {
        if (!valueBeingRemoved.isNull())
            willModifyAttribute(name, valueBeingRemoved, nullAtom);
    }

    attributes.remove(index);

    if (!inSynchronizationOfLazyAttribute)
        attributeChanged(name, nullAtom);
}

void Element::appendAttributeInternal(const QualifiedName& name, const AtomicString& value, SynchronizationOfLazyAttribute inSynchronizationOfLazyAttribute)
{
    if (!inSynchronizationOfLazyAttribute)
        willModifyAttribute(name, nullAtom, value);
    ensureUniqueElementData().attributes().append(name, value);
    if (!inSynchronizationOfLazyAttribute)
        attributeChanged(name, value);
}

void Element::removeAttribute(const AtomicString& localName)
{
    if (!elementData())
        return;

    size_t index = elementData()->attributes().findIndex(localName);
    if (index == kNotFound) {
        if (UNLIKELY(localName == HTMLNames::styleAttr) && elementData()->m_styleAttributeIsDirty && isStyledElement())
            removeAllInlineStyleProperties();
        return;
    }

    removeAttributeInternal(index, NotInSynchronizationOfLazyAttribute);
}

Vector<RefPtr<Attr>> Element::getAttributes()
{
    if (!elementData())
        return Vector<RefPtr<Attr>>();
    synchronizeAllAttributes();
    Vector<RefPtr<Attr>> attributes;
    for (const Attribute& attribute : elementData()->attributes())
        attributes.append(Attr::create(attribute.name(), attribute.value()));
    return attributes;
}

RenderStyle* Element::computedStyle()
{
    // FIXME: Find and use the renderer from the pseudo element instead of the actual element so that the 'length'
    // properties, which are only known by the renderer because it did the layout, will be correct and so that the
    // values returned for the ":selection" pseudo-element will be correct.
    if (RenderStyle* usedStyle = renderStyle())
        return usedStyle;

    if (!inActiveDocument())
        // FIXME: Try to do better than this. Ensure that styleForElement() works for elements that are not in the
        // document tree and figure out when to destroy the computed style for such elements.
        return 0;

    ElementRareData& rareData = ensureElementRareData();
    if (!rareData.computedStyle()) {
        RenderStyle* parentStyle = parentNode() ? parentNode()->computedStyle() : 0;
        rareData.setComputedStyle(document().styleResolver().styleForElement(this, parentStyle));
    }
    return rareData.computedStyle();
}

AtomicString Element::computeInheritedLanguage() const
{
    const Node* n = this;
    AtomicString value;
    // The language property is inherited, so we iterate over the parents to find the first language.
    do {
        if (n->isElementNode()) {
            if (const ElementData* elementData = toElement(n)->elementData()) {
                AttributeCollection attributes = elementData->attributes();
                if (const Attribute* attribute = attributes.find(HTMLNames::langAttr))
                    value = attribute->value();
            }
        } else if (n->isDocumentNode()) {
            // checking the MIME content-language
            value = toDocument(n)->contentLanguage();
        }

        n = n->parentNode();
    } while (n && value.isNull());

    return value;
}

bool Element::matches(const String& selectors, ExceptionState& exceptionState)
{
    SelectorQuery* selectorQuery = document().selectorQueryCache().add(AtomicString(selectors), document(), exceptionState);
    if (!selectorQuery)
        return false;
    return selectorQuery->matches(*this);
}

DOMTokenList& Element::classList()
{
    ElementRareData& rareData = ensureElementRareData();
    if (!rareData.classList())
        rareData.setClassList(DOMTokenList::create(*this));
    return *rareData.classList();
}

KURL Element::hrefURL() const
{
    return KURL();
}

KURL Element::getURLAttribute(const QualifiedName& name) const
{
#if ENABLE(ASSERT)
    if (elementData()) {
        if (const Attribute* attribute = attributes().find(name))
            ASSERT(isURLAttribute(*attribute));
    }
#endif
    return document().completeURL(stripLeadingAndTrailingHTMLSpaces(getAttribute(name)));
}

KURL Element::getNonEmptyURLAttribute(const QualifiedName& name) const
{
#if ENABLE(ASSERT)
    if (elementData()) {
        if (const Attribute* attribute = attributes().find(name))
            ASSERT(isURLAttribute(*attribute));
    }
#endif
    String value = stripLeadingAndTrailingHTMLSpaces(getAttribute(name));
    if (value.isEmpty())
        return KURL();
    return document().completeURL(value);
}

int Element::getIntegralAttribute(const QualifiedName& attributeName) const
{
    return getAttribute(attributeName).string().toInt();
}

void Element::setIntegralAttribute(const QualifiedName& attributeName, int value)
{
    setAttribute(attributeName, AtomicString::number(value));
}

unsigned Element::getUnsignedIntegralAttribute(const QualifiedName& attributeName) const
{
    return getAttribute(attributeName).string().toUInt();
}

void Element::setUnsignedIntegralAttribute(const QualifiedName& attributeName, unsigned value)
{
    // Range restrictions are enforced for unsigned IDL attributes that
    // reflect content attributes,
    //   http://www.whatwg.org/specs/web-apps/current-work/multipage/common-dom-interfaces.html#reflecting-content-attributes-in-idl-attributes
    if (value > 0x7fffffffu)
        value = 0;
    setAttribute(attributeName, AtomicString::number(value));
}

double Element::getFloatingPointAttribute(const QualifiedName& attributeName, double fallbackValue) const
{
    return parseToDoubleForNumberType(getAttribute(attributeName), fallbackValue);
}

void Element::setFloatingPointAttribute(const QualifiedName& attributeName, double value)
{
    setAttribute(attributeName, AtomicString::number(value));
}

SpellcheckAttributeState Element::spellcheckAttributeState() const
{
    const AtomicString& value = getAttribute(HTMLNames::spellcheckAttr);
    if (value == nullAtom)
        return SpellcheckAttributeDefault;
    if (equalIgnoringCase(value, "true") || equalIgnoringCase(value, ""))
        return SpellcheckAttributeTrue;
    if (equalIgnoringCase(value, "false"))
        return SpellcheckAttributeFalse;

    return SpellcheckAttributeDefault;
}

bool Element::isSpellCheckingEnabled() const
{
    for (const Element* element = this; element; element = element->parentElement()) {
        switch (element->spellcheckAttributeState()) {
        case SpellcheckAttributeTrue:
            return true;
        case SpellcheckAttributeFalse:
            return false;
        case SpellcheckAttributeDefault:
            break;
        }
    }

    return true;
}

inline void Element::updateId(const AtomicString& oldId, const AtomicString& newId)
{
    if (!isInTreeScope())
        return;

    if (oldId == newId)
        return;

    updateId(treeScope(), oldId, newId);
}

inline void Element::updateId(TreeScope& scope, const AtomicString& oldId, const AtomicString& newId)
{
    ASSERT(isInTreeScope());
    ASSERT(oldId != newId);

    if (!oldId.isEmpty())
        scope.removeElementById(oldId, this);
    if (!newId.isEmpty())
        scope.addElementById(newId, this);
}

void Element::willModifyAttribute(const QualifiedName& name, const AtomicString& oldValue, const AtomicString& newValue)
{
    if (name == HTMLNames::idAttr)
        updateId(oldValue, newValue);

    if (inActiveDocument() && styleChangeType() < SubtreeStyleChange && affectedByAttributeSelector(name.localName()))
        setNeedsStyleRecalc(LocalStyleChange);

    if (OwnPtr<MutationObserverInterestGroup> recipients = MutationObserverInterestGroup::createForAttributesMutation(*this, name))
        recipients->enqueueMutationRecord(MutationRecord::createAttributes(this, name, oldValue));
}

void Element::didMoveToNewDocument(Document& oldDocument)
{
    Node::didMoveToNewDocument(oldDocument);
}

void Element::cloneAttributesFromElement(const Element& other)
{
    other.synchronizeAllAttributes();
    if (!other.m_elementData) {
        m_elementData.clear();
        return;
    }

    const AtomicString& oldID = getIdAttribute();
    const AtomicString& newID = other.getIdAttribute();

    if (!oldID.isNull() || !newID.isNull())
        updateId(oldID, newID);

    // If 'other' has a mutable ElementData, convert it to an immutable one so we can share it between both elements.
    // We can only do this if there are no presentation attributes and sharing the data won't result in different case sensitivity of class or id.
    if (other.m_elementData->isUnique())
        const_cast<Element&>(other).m_elementData = toUniqueElementData(other.m_elementData)->makeShareableCopy();

    if (!other.m_elementData->isUnique())
        m_elementData = other.m_elementData;
    else
        m_elementData = other.m_elementData->makeUniqueCopy();

    AttributeCollection attributes = m_elementData->attributes();
    AttributeCollection::iterator end = attributes.end();
    for (AttributeCollection::iterator it = attributes.begin(); it != end; ++it)
        attributeChangedFromParserOrByCloning(it->name(), it->value(), ModifiedByCloning);
}

void Element::cloneDataFromElement(const Element& other)
{
    // FIXME(sky): Merge these.
    cloneAttributesFromElement(other);
}

void Element::createUniqueElementData()
{
    if (!m_elementData)
        m_elementData = UniqueElementData::create();
    else {
        ASSERT(!m_elementData->isUnique());
        m_elementData = toShareableElementData(m_elementData)->makeUniqueCopy();
    }
}

void Element::synchronizeStyleAttributeInternal() const
{
    ASSERT(isStyledElement());
    ASSERT(elementData());
    ASSERT(elementData()->m_styleAttributeIsDirty);
    elementData()->m_styleAttributeIsDirty = false;
    const StylePropertySet* inlineStyle = this->inlineStyle();
    const_cast<Element*>(this)->setSynchronizedLazyAttribute(HTMLNames::styleAttr,
        inlineStyle ? AtomicString(inlineStyle->asText()) : nullAtom);
}

CSSStyleDeclaration* Element::style()
{
    if (!isStyledElement())
        return 0;
    return &ensureElementRareData().ensureInlineCSSStyleDeclaration(this);
}

MutableStylePropertySet& Element::ensureMutableInlineStyle()
{
    ASSERT(isStyledElement());
    RefPtr<StylePropertySet>& inlineStyle = ensureUniqueElementData().m_inlineStyle;
    if (!inlineStyle) {
        inlineStyle = MutableStylePropertySet::create(HTMLStandardMode);
    } else if (!inlineStyle->isMutable()) {
        inlineStyle = inlineStyle->mutableCopy();
    }
    return *toMutableStylePropertySet(inlineStyle);
}

void Element::clearMutableInlineStyleIfEmpty()
{
    if (ensureMutableInlineStyle().isEmpty()) {
        ensureUniqueElementData().m_inlineStyle.clear();
    }
}

inline void Element::setInlineStyleFromString(const AtomicString& newStyleString)
{
    ASSERT(isStyledElement());
    RefPtr<StylePropertySet>& inlineStyle = elementData()->m_inlineStyle;

    // Avoid redundant work if we're using shared attribute data with already parsed inline style.
    if (inlineStyle && !elementData()->isUnique())
        return;

    // We reconstruct the property set instead of mutating if there is no CSSOM wrapper.
    // This makes wrapperless property sets immutable and so cacheable.
    if (inlineStyle && !inlineStyle->isMutable())
        inlineStyle.clear();

    if (!inlineStyle) {
        inlineStyle = BisonCSSParser::parseInlineStyleDeclaration(newStyleString, this);
    } else {
        ASSERT(inlineStyle->isMutable());
        static_cast<MutableStylePropertySet*>(inlineStyle.get())->parseDeclaration(newStyleString, document().elementSheet().contents());
    }
}

void Element::styleAttributeChanged(const AtomicString& newStyleString)
{
    ASSERT(isStyledElement());

    if (newStyleString.isNull()) {
        ensureUniqueElementData().m_inlineStyle.clear();
    } else {
        setInlineStyleFromString(newStyleString);
    }

    elementData()->m_styleAttributeIsDirty = false;

    setNeedsStyleRecalc(LocalStyleChange);
}

void Element::inlineStyleChanged()
{
    ASSERT(isStyledElement());
    setNeedsStyleRecalc(LocalStyleChange);
    ASSERT(elementData());
    elementData()->m_styleAttributeIsDirty = true;
}

bool Element::setInlineStyleProperty(CSSPropertyID propertyID, CSSValueID identifier)
{
    ASSERT(isStyledElement());
    ensureMutableInlineStyle().setProperty(propertyID, cssValuePool().createIdentifierValue(identifier));
    inlineStyleChanged();
    return true;
}

bool Element::setInlineStyleProperty(CSSPropertyID propertyID, double value, CSSPrimitiveValue::UnitType unit)
{
    ASSERT(isStyledElement());
    ensureMutableInlineStyle().setProperty(propertyID, cssValuePool().createValue(value, unit));
    inlineStyleChanged();
    return true;
}

bool Element::setInlineStyleProperty(CSSPropertyID propertyID, const String& value)
{
    ASSERT(isStyledElement());
    bool changes = ensureMutableInlineStyle().setProperty(propertyID, value, document().elementSheet().contents());
    if (changes)
        inlineStyleChanged();
    return changes;
}

bool Element::removeInlineStyleProperty(CSSPropertyID propertyID)
{
    ASSERT(isStyledElement());
    if (!inlineStyle())
        return false;
    bool changes = ensureMutableInlineStyle().removeProperty(propertyID);
    if (changes)
        inlineStyleChanged();
    return changes;
}

void Element::removeAllInlineStyleProperties()
{
    ASSERT(isStyledElement());
    if (!inlineStyle())
        return;
    ensureMutableInlineStyle().clear();
    inlineStyleChanged();
}

bool Element::supportsStyleSharing() const
{
    if (!isStyledElement() || !parentElement())
        return false;
    // If the element has inline style it is probably unique.
    if (inlineStyle())
        return false;
    // Ids stop style sharing if they show up in the stylesheets.
    if (hasID() && affectedByIdSelector(idForStyleResolution()))
        return false;
    // :active and :hover elements always make a chain towards the document node
    // and no siblings or cousins will have the same state.
    if (isUserActionElement())
        return false;
    return true;
}

bool Element::affectedByAttributeSelector(const AtomicString& attributeName) const
{
    if (attributeName.isEmpty())
        return false;
    if (treeScope().scopedStyleResolver().hasSelectorForAttribute(attributeName))
        return true;
    return false;
}

bool Element::affectedByClassSelector(const AtomicString& classValue) const
{
    if (classValue.isEmpty())
        return false;
    if (treeScope().scopedStyleResolver().hasSelectorForClass(classValue))
        return true;
    return false;
}

bool Element::affectedByIdSelector(const AtomicString& idValue) const
{
    if (idValue.isEmpty())
        return false;
    if (treeScope().scopedStyleResolver().hasSelectorForId(idValue))
        return true;
    return false;
}

void Element::paint(Canvas* canvas)
{
    if (!renderer() || !renderer()->isBox())
        return;
    RenderBox* box = toRenderBox(renderer());
    GraphicsContext context(canvas->skCanvas());

    // Very simplified painting to allow painting an arbitrary (layer-less) subtree.
    Vector<RenderBox*> layers;
    PaintInfo paintInfo(&context, box->absoluteBoundingBoxRect(), box);
    box->paint(paintInfo, LayoutPoint(), layers);
    // Note we're ignoring any layers encountered.
}

} // namespace blink
