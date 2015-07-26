/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003-2011, 2013, 2014 Apple Inc. All rights reserved.
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
 *
 */

#ifndef SKY_ENGINE_CORE_DOM_ELEMENT_H_
#define SKY_ENGINE_CORE_DOM_ELEMENT_H_

#include "gen/sky/core/CSSPropertyNames.h"
#include "gen/sky/core/HTMLNames.h"
#include "sky/engine/core/css/CSSPrimitiveValue.h"
#include "sky/engine/core/dom/Attribute.h"
#include "sky/engine/core/dom/ContainerNode.h"
#include "sky/engine/core/dom/ElementData.h"
#include "sky/engine/core/dom/SpaceSplitString.h"
#include "sky/engine/platform/heap/Handle.h"

namespace blink {

class Attr;
class Attribute;
class CSSStyleDeclaration;
class Canvas;
class ClientRect;
class ClientRectList;
class DOMTokenList;
class Document;
class ElementRareData;
class ExceptionState;
class Image;
class IntSize;
class LayoutCallback;
class MutableStylePropertySet;
class PaintingCallback;
class PropertySetCSSStyleDeclaration;
class PseudoElement;
class StylePropertySet;

enum SpellcheckAttributeState {
    SpellcheckAttributeTrue,
    SpellcheckAttributeFalse,
    SpellcheckAttributeDefault
};

class Element : public ContainerNode {
    DEFINE_WRAPPERTYPEINFO();
public:
    static PassRefPtr<Element> create(const QualifiedName&, Document*);
    virtual ~Element();

    bool hasAttribute(const QualifiedName&) const;
    const AtomicString& getAttribute(const QualifiedName&) const;
    Vector<RefPtr<Attr>> getAttributes();
    bool hasAttributes() const;
    bool hasAttribute(const AtomicString& name) const;
    const AtomicString& getAttribute(const AtomicString& name) const;
    void setAttribute(const AtomicString& name, const AtomicString& value, ExceptionState&);
    void setAttribute(const AtomicString& name, ExceptionState& es) {
        setAttribute(name, String(), es);
    }
    void removeAttribute(const AtomicString& name);
    void removeAttribute(const QualifiedName&);

    // Passing nullAtom as the second parameter removes the attribute when calling either of these set methods.
    void setAttribute(const QualifiedName&, const AtomicString& value);
    void setSynchronizedLazyAttribute(const QualifiedName&, const AtomicString& value);

    // Typed getters and setters for language bindings.
    int getIntegralAttribute(const QualifiedName& attributeName) const;
    void setIntegralAttribute(const QualifiedName& attributeName, int value);
    unsigned getUnsignedIntegralAttribute(const QualifiedName& attributeName) const;
    void setUnsignedIntegralAttribute(const QualifiedName& attributeName, unsigned value);
    double getFloatingPointAttribute(const QualifiedName& attributeName, double fallbackValue = std::numeric_limits<double>::quiet_NaN()) const;
    void setFloatingPointAttribute(const QualifiedName& attributeName, double value);

    const AtomicString& getIdAttribute() const;
    void setIdAttribute(const AtomicString&);

    const AtomicString& getClassAttribute() const;

    // Call this to get the value of the id attribute for style resolution purposes.
    // The value will already be lowercased if the document is in compatibility mode,
    // so this function is not suitable for non-style uses.
    const AtomicString& idForStyleResolution() const;

    // This getter takes care of synchronizing all attributes before returning the
    // AttributeCollection. If the Element has no attributes, an empty AttributeCollection
    // will be returned. This is not a trivial getter and its return value should be cached
    // for performance.
    AttributeCollection attributes() const;
    // This variant will not update the potentially invalid attributes. To be used when not interested
    // in style attribute.
    AttributeCollection attributesWithoutUpdate() const;

    int offsetLeft();
    int offsetTop();
    int offsetWidth();
    int offsetHeight();

    Element* offsetParent();
    int clientLeft();
    int clientTop();
    int clientWidth();
    int clientHeight();

    PassRefPtr<ClientRectList> getClientRects();
    PassRefPtr<ClientRect> getBoundingClientRect();

    void requestPaint(PassOwnPtr<PaintingCallback>);

    virtual void didMoveToNewDocument(Document&) override;

    CSSStyleDeclaration* style();

    const QualifiedName& tagQName() const { return m_tagName; }
    String tagName() const { return nodeName(); }

    bool hasTagName(const QualifiedName& tagName) const { return m_tagName == tagName; }

    // A fast function for checking the local name against another atomic string.
    bool hasLocalName(const AtomicString& other) const { return m_tagName.localName() == other; }

    virtual const AtomicString& localName() const override final { return m_tagName.localName(); }

    virtual String nodeName() const override;

    PassRefPtr<Element> cloneElementWithChildren();
    PassRefPtr<Element> cloneElementWithoutChildren();

    void setBooleanAttribute(const QualifiedName& name, bool);

    void invalidateStyleAttribute();

    bool affectedByAttributeSelector(const AtomicString& attributeName) const;
    bool affectedByClassSelector(const AtomicString& classValue) const;
    bool affectedByIdSelector(const AtomicString& idValue) const;

    const StylePropertySet* inlineStyle() const { return elementData() ? elementData()->m_inlineStyle.get() : 0; }

    bool setInlineStyleProperty(CSSPropertyID, CSSValueID identifier);
    bool setInlineStyleProperty(CSSPropertyID, double value, CSSPrimitiveValue::UnitType);
    bool setInlineStyleProperty(CSSPropertyID, const String& value);
    bool removeInlineStyleProperty(CSSPropertyID);
    void removeAllInlineStyleProperties();

    void synchronizeStyleAttributeInternal() const;

    enum AttributeModificationReason {
        ModifiedDirectly,
        ModifiedByCloning
    };

    // Only called by the parser immediately after element construction.
    void parserSetAttributes(const Vector<Attribute>&);

    bool sharesSameElementData(const Element& other) const { return elementData() == other.elementData(); }

    // Clones attributes only.
    void cloneAttributesFromElement(const Element&);

    // Clones all attribute-derived data, including subclass specifics (through copyNonAttributeProperties.)
    void cloneDataFromElement(const Element&);

    bool hasEquivalentAttributes(const Element* other) const;

    void attach(const AttachContext& = AttachContext()) final;
    void detach(const AttachContext& = AttachContext()) final;

    virtual RenderObject* createRenderer(RenderStyle*);
    void recalcStyle(StyleRecalcChange);

    bool supportsStyleSharing() const;

    double x() const;
    void setX(double);

    double y() const;
    void setY(double);

    double width() const;
    void setWidth(double);

    double height() const;
    void setHeight(double);

    double minContentWidth() const;
    void setMinContentWidth(double);

    double maxContentWidth() const;
    void setMaxContentWidth(double);

    double alphabeticBaseline() const;
    double ideographicBaseline() const;

    void setNeedsLayout();
    void layout();

    RenderStyle* computedStyle();

    AtomicString computeInheritedLanguage() const;

    virtual bool isURLAttribute(const Attribute&) const { return false; }

    virtual bool isLiveLink() const { return false; }
    KURL hrefURL() const;

    KURL getURLAttribute(const QualifiedName&) const;
    KURL getNonEmptyURLAttribute(const QualifiedName&) const;

    virtual const AtomicString imageSourceURL() const;

    bool matches(const String& selectors, ExceptionState&);

    DOMTokenList& classList();

    virtual bool canContainRangeEndPoint() const override { return true; }

    bool isSpellCheckingEnabled() const;

    // FIXME: public for RenderTreeBuilder, we shouldn't expose this though.
    PassRefPtr<RenderStyle> styleForRenderer();

    bool hasID() const;
    bool hasClass() const;
    const SpaceSplitString& classNames() const;

    void synchronizeAttribute(const AtomicString& localName) const;

    MutableStylePropertySet& ensureMutableInlineStyle();
    void clearMutableInlineStyleIfEmpty();

    String contentEditable() const;
    void setContentEditable(const String&, ExceptionState&);

    bool spellcheck() const;
    void setSpellcheck(bool);

    const AtomicString& dir();
    void setDir(const AtomicString&);

    // Dart exposed:
    void paint(Canvas*);

protected:
    Element(const QualifiedName& tagName, Document*, ConstructionType);

    const ElementData* elementData() const { return m_elementData.get(); }
    UniqueElementData& ensureUniqueElementData();

    virtual void insertedInto(ContainerNode*) override;
    virtual void removedFrom(ContainerNode*) override;
    virtual void childrenChanged(const ChildrenChange&) override;

    // classAttributeChanged() exists to share code between
    // parseAttribute (called via setAttribute()) and
    // svgAttributeChanged (called when element.className.baseValue is set)
    void classAttributeChanged(const AtomicString& newClassString);

private:
    void attributeChanged(const QualifiedName&, const AtomicString&, AttributeModificationReason = ModifiedDirectly);

    bool classChangeNeedsStyleRecalc(const SpaceSplitString& oldClasses, const SpaceSplitString& newClasses);

    bool isElementNode() const = delete; // This will catch anyone doing an unnecessary check.
    bool isDocumentFragment() const = delete; // This will catch anyone doing an unnecessary check.
    bool isDocumentNode() const = delete; // This will catch anyone doing an unnecessary check.

    void styleAttributeChanged(const AtomicString& newStyleString);

    void inlineStyleChanged();
    PropertySetCSSStyleDeclaration* inlineStyleCSSOMWrapper();
    void setInlineStyleFromString(const AtomicString&);

    StyleRecalcChange recalcOwnStyle(StyleRecalcChange);
    void recalcChildStyle(StyleRecalcChange);

    enum SynchronizationOfLazyAttribute { NotInSynchronizationOfLazyAttribute = 0, InSynchronizationOfLazyAttribute };

    void willModifyAttribute(const QualifiedName&, const AtomicString& oldValue, const AtomicString& newValue);

    void synchronizeAllAttributes() const;

    void updateId(const AtomicString& oldId, const AtomicString& newId);
    void updateId(TreeScope&, const AtomicString& oldId, const AtomicString& newId);

    virtual NodeType nodeType() const override final;

    void setAttributeInternal(size_t index, const QualifiedName&, const AtomicString& value, SynchronizationOfLazyAttribute);
    void appendAttributeInternal(const QualifiedName&, const AtomicString& value, SynchronizationOfLazyAttribute);
    void removeAttributeInternal(size_t index, SynchronizationOfLazyAttribute);
    void attributeChangedFromParserOrByCloning(const QualifiedName&, const AtomicString&, AttributeModificationReason);

#ifndef NDEBUG
    virtual void formatForDebugger(char* buffer, unsigned length) const override;
#endif

    virtual RenderStyle* virtualComputedStyle() override { return computedStyle(); }

    // cloneNode is private so that non-virtual cloneElementWithChildren and cloneElementWithoutChildren
    // are used instead.
    virtual PassRefPtr<Node> cloneNode(bool deep) override;
    virtual PassRefPtr<Element> cloneElementWithoutAttributesAndChildren();

    QualifiedName m_tagName;

    SpellcheckAttributeState spellcheckAttributeState() const;

    void createUniqueElementData();

    ElementRareData* elementRareData() const;
    ElementRareData& ensureElementRareData();

    RefPtr<ElementData> m_elementData;
};

DEFINE_NODE_TYPE_CASTS(Element, isElementNode());
template <typename T> bool isElementOfType(const Node&);
template <> inline bool isElementOfType<const Element>(const Node& node) { return node.isElementNode(); }
template <typename T> inline bool isElementOfType(const Element& element) { return isElementOfType<T>(static_cast<const Node&>(element)); }
template <> inline bool isElementOfType<const Element>(const Element&) { return true; }

// Type casting.
template<typename T> inline T& toElement(Node& node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(isElementOfType<const T>(node));
    return static_cast<T&>(node);
}
template<typename T> inline T* toElement(Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isElementOfType<const T>(*node));
    return static_cast<T*>(node);
}
template<typename T> inline const T& toElement(const Node& node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(isElementOfType<const T>(node));
    return static_cast<const T&>(node);
}
template<typename T> inline const T* toElement(const Node* node)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!node || isElementOfType<const T>(*node));
    return static_cast<const T*>(node);
}
template<typename T, typename U> inline T* toElement(const RefPtr<U>& node) { return toElement<T>(node.get()); }

inline Element* Node::parentElement() const
{
    ContainerNode* parent = parentNode();
    return parent && parent->isElementNode() ? toElement(parent) : 0;
}

inline void Element::synchronizeAttribute(const AtomicString& localName) const
{
    if (!elementData())
        return;
    if (localName == HTMLNames::styleAttr && elementData()->m_styleAttributeIsDirty) {
        ASSERT(isStyledElement());
        synchronizeStyleAttributeInternal();
    }
}

inline bool Element::hasAttribute(const QualifiedName& name) const
{
    return hasAttribute(name.localName());
}

inline bool Element::hasAttribute(const AtomicString& name) const
{
    synchronizeAttribute(name);
    return elementData() && elementData()->attributes().findIndex(name) != kNotFound;
}

inline const AtomicString& Element::getAttribute(const QualifiedName& name) const
{
    return getAttribute(name.localName());
}

inline const AtomicString& Element::getAttribute(const AtomicString& name) const
{
    if (!elementData())
        return nullAtom;
    synchronizeAttribute(name);
    if (const Attribute* attribute = elementData()->attributes().find(name))
        return attribute->value();
    return nullAtom;
}

inline AttributeCollection Element::attributes() const
{
    if (!elementData())
        return AttributeCollection();
    synchronizeAllAttributes();
    return elementData()->attributes();
}

inline AttributeCollection Element::attributesWithoutUpdate() const
{
    if (!elementData())
        return AttributeCollection();
    return elementData()->attributes();
}

inline bool Element::hasAttributes() const
{
    return !attributes().isEmpty();
}

inline const AtomicString& Element::idForStyleResolution() const
{
    ASSERT(hasID());
    return elementData()->idForStyleResolution();
}

inline const AtomicString& Element::getIdAttribute() const
{
    return hasID() ? getAttribute(HTMLNames::idAttr) : nullAtom;
}

inline const AtomicString& Element::getClassAttribute() const
{
    if (!hasClass())
        return nullAtom;
    return getAttribute(HTMLNames::classAttr);
}

inline void Element::setIdAttribute(const AtomicString& value)
{
    setAttribute(HTMLNames::idAttr, value);
}

inline const SpaceSplitString& Element::classNames() const
{
    ASSERT(hasClass());
    ASSERT(elementData());
    return elementData()->classNames();
}

inline bool Element::hasID() const
{
    return elementData() && elementData()->hasID();
}

inline bool Element::hasClass() const
{
    return elementData() && elementData()->hasClass();
}

inline UniqueElementData& Element::ensureUniqueElementData()
{
    if (!elementData() || !elementData()->isUnique())
        createUniqueElementData();
    return toUniqueElementData(*m_elementData);
}

inline void Node::insertedInto(ContainerNode* insertionPoint)
{
    ASSERT(insertionPoint->inDocument() || isContainerNode());
    if (insertionPoint->inDocument())
        setFlag(InDocumentFlag);
}

inline void Node::removedFrom(ContainerNode* insertionPoint)
{
    ASSERT(insertionPoint->inDocument() || isContainerNode());
    if (insertionPoint->inDocument())
        clearFlag(InDocumentFlag);
}

inline void Element::invalidateStyleAttribute()
{
    ASSERT(elementData());
    elementData()->m_styleAttributeIsDirty = true;
}

// These macros do the same as their NODE equivalents but additionally provide a template specialization
// for isElementOfType<>() so that the Traversal<> API works for these Element types.
#define DEFINE_ELEMENT_TYPE_CASTS(thisType, predicate) \
    template <> inline bool isElementOfType<const thisType>(const Node& node) { return node.predicate; } \
    DEFINE_NODE_TYPE_CASTS(thisType, predicate)

#define DEFINE_ELEMENT_TYPE_CASTS_WITH_FUNCTION(thisType) \
    template <> inline bool isElementOfType<const thisType>(const Node& node) { return is##thisType(node); } \
    DEFINE_NODE_TYPE_CASTS_WITH_FUNCTION(thisType)

#define DECLARE_ELEMENT_FACTORY_WITH_TAGNAME(T) \
    static PassRefPtr<T> create(const QualifiedName&, Document&)
#define DEFINE_ELEMENT_FACTORY_WITH_TAGNAME(T) \
    PassRefPtr<T> T::create(const QualifiedName& tagName, Document& document) \
    { \
        return adoptRef(new T(tagName, document)); \
    }

} // namespace

#endif  // SKY_ENGINE_CORE_DOM_ELEMENT_H_
