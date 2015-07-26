/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2008, 2009, 2011, 2012 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) Research In Motion Limited 2010-2011. All rights reserved.
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

#include "sky/engine/core/dom/Document.h"

#include "gen/sky/core/EventNames.h"
#include "gen/sky/platform/RuntimeEnabledFeatures.h"
#include "sky/engine/bindings/exception_messages.h"
#include "sky/engine/bindings/exception_state.h"
#include "sky/engine/bindings/exception_state_placeholder.h"
#include "sky/engine/core/css/CSSFontSelector.h"
#include "sky/engine/core/css/CSSStyleDeclaration.h"
#include "sky/engine/core/css/CSSStyleSheet.h"
#include "sky/engine/core/css/MediaQueryMatcher.h"
#include "sky/engine/core/css/StylePropertySet.h"
#include "sky/engine/core/css/StyleSheetContents.h"
#include "sky/engine/core/css/parser/BisonCSSParser.h"
#include "sky/engine/core/css/resolver/FontBuilder.h"
#include "sky/engine/core/css/resolver/StyleResolver.h"
#include "sky/engine/core/css/resolver/StyleResolverStats.h"
#include "sky/engine/core/dom/Attr.h"
#include "sky/engine/core/dom/DocumentFragment.h"
#include "sky/engine/core/dom/DocumentMarkerController.h"
#include "sky/engine/core/dom/Element.h"
#include "sky/engine/core/dom/ElementDataCache.h"
#include "sky/engine/core/dom/ElementTraversal.h"
#include "sky/engine/core/dom/ExceptionCode.h"
#include "sky/engine/core/dom/MutationObserver.h"
#include "sky/engine/core/dom/NodeRareData.h"
#include "sky/engine/core/dom/NodeRenderStyle.h"
#include "sky/engine/core/dom/NodeTraversal.h"
#include "sky/engine/core/dom/NodeWithIndex.h"
#include "sky/engine/core/dom/Range.h"
#include "sky/engine/core/dom/RequestAnimationFrameCallback.h"
#include "sky/engine/core/dom/ScriptedAnimationController.h"
#include "sky/engine/core/dom/SelectorQuery.h"
#include "sky/engine/core/dom/StaticNodeList.h"
#include "sky/engine/core/dom/StyleEngine.h"
#include "sky/engine/core/dom/Text.h"
#include "sky/engine/core/editing/PositionWithAffinity.h"
#include "sky/engine/core/events/Event.h"
#include "sky/engine/core/events/PageTransitionEvent.h"
#include "sky/engine/core/frame/FrameHost.h"
#include "sky/engine/core/frame/FrameView.h"
#include "sky/engine/core/frame/LocalDOMWindow.h"
#include "sky/engine/core/frame/LocalFrame.h"
#include "sky/engine/core/frame/Settings.h"
#include "sky/engine/core/inspector/ConsoleMessage.h"
#include "sky/engine/core/inspector/InspectorCounters.h"
#include "sky/engine/core/loader/FrameLoaderClient.h"
#include "sky/engine/core/page/ChromeClient.h"
#include "sky/engine/core/page/Page.h"
#include "sky/engine/core/painting/PaintingTasks.h"
#include "sky/engine/core/painting/Picture.h"
#include "sky/engine/core/rendering/HitTestResult.h"
#include "sky/engine/core/rendering/RenderView.h"
#include "sky/engine/platform/DateComponents.h"
#include "sky/engine/platform/EventDispatchForbiddenScope.h"
#include "sky/engine/platform/Language.h"
#include "sky/engine/platform/Logging.h"
#include "sky/engine/platform/ScriptForbiddenScope.h"
#include "sky/engine/platform/TraceEvent.h"
#include "sky/engine/platform/text/SegmentedString.h"
#include "sky/engine/public/platform/Platform.h"
#include "sky/engine/wtf/CurrentTime.h"
#include "sky/engine/wtf/DateMath.h"
#include "sky/engine/wtf/HashFunctions.h"
#include "sky/engine/wtf/MainThread.h"
#include "sky/engine/wtf/StdLibExtras.h"
#include "sky/engine/wtf/TemporaryChange.h"
#include "sky/engine/wtf/text/StringBuffer.h"

using namespace WTF;
using namespace Unicode;

namespace blink {

// DOM Level 2 says (letters added):
//
// a) Name start characters must have one of the categories Ll, Lu, Lo, Lt, Nl.
// b) Name characters other than Name-start characters must have one of the categories Mc, Me, Mn, Lm, or Nd.
// c) Characters in the compatibility area (i.e. with character code greater than #xF900 and less than #xFFFE) are not allowed in XML names.
// d) Characters which have a font or compatibility decomposition (i.e. those with a "compatibility formatting tag" in field 5 of the database -- marked by field 5 beginning with a "<") are not allowed.
// e) The following characters are treated as name-start characters rather than name characters, because the property file classifies them as Alphabetic: [#x02BB-#x02C1], #x0559, #x06E5, #x06E6.
// f) Characters #x20DD-#x20E0 are excluded (in accordance with Unicode, section 5.14).
// g) Character #x00B7 is classified as an extender, because the property list so identifies it.
// h) Character #x0387 is added as a name character, because #x00B7 is its canonical equivalent.
// i) Characters ':' and '_' are allowed as name-start characters.
// j) Characters '-' and '.' are allowed as name characters.
//
// It also contains complete tables. If we decide it's better, we could include those instead of the following code.

static inline bool isValidNameStart(UChar32 c)
{
    // rule (e) above
    if ((c >= 0x02BB && c <= 0x02C1) || c == 0x559 || c == 0x6E5 || c == 0x6E6)
        return true;

    // rule (i) above
    if (c == ':' || c == '_')
        return true;

    // rules (a) and (f) above
    const uint32_t nameStartMask = Letter_Lowercase | Letter_Uppercase | Letter_Other | Letter_Titlecase | Number_Letter;
    if (!(Unicode::category(c) & nameStartMask))
        return false;

    // rule (c) above
    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    // rule (d) above
    DecompositionType decompType = decompositionType(c);
    if (decompType == DecompositionFont || decompType == DecompositionCompat)
        return false;

    return true;
}

static inline bool isValidNamePart(UChar32 c)
{
    // rules (a), (e), and (i) above
    if (isValidNameStart(c))
        return true;

    // rules (g) and (h) above
    if (c == 0x00B7 || c == 0x0387)
        return true;

    // rule (j) above
    if (c == '-' || c == '.')
        return true;

    // rules (b) and (f) above
    const uint32_t otherNamePartMask = Mark_NonSpacing | Mark_Enclosing | Mark_SpacingCombining | Letter_Modifier | Number_DecimalDigit;
    if (!(Unicode::category(c) & otherNamePartMask))
        return false;

    // rule (c) above
    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    // rule (d) above
    DecompositionType decompType = decompositionType(c);
    if (decompType == DecompositionFont || decompType == DecompositionCompat)
        return false;

    return true;
}

#ifndef NDEBUG
typedef HashSet<RawPtr<Document> > WeakDocumentSet;
static WeakDocumentSet& liveDocumentSet()
{
    DEFINE_STATIC_LOCAL(OwnPtr<WeakDocumentSet>, set, (adoptPtr(new WeakDocumentSet())));
    return *set;
}
#endif

Document::Document(const DocumentInit& initializer)
    : ContainerNode(0, CreateDocument)
    , TreeScope(*this)
    , m_active(false)
    , m_visualUpdatePending(true)
    , m_inStyleRecalc(false)
    , m_stopped(false)
    , m_module(nullptr)
    , m_evaluateMediaQueriesOnStyleRecalc(false)
    , m_frame(initializer.frame())
    , m_domWindow(m_frame ? m_frame->domWindow() : 0)
    , m_activeParserCount(0)
    , m_listenerTypes(0)
    , m_mutationObserverTypes(0)
    , m_readyState(Complete)
    , m_isParsing(false)
    , m_containsValidityStyleRules(false)
    , m_markers(adoptPtr(new DocumentMarkerController))
    , m_loadEventProgress(LoadEventNotRun)
    , m_startTime(currentTime())
    , m_renderView(0)
#if !ENABLE(OILPAN)
    , m_weakFactory(this)
#endif
    , m_didSetReferrerPolicy(false)
    , m_referrerPolicy(ReferrerPolicyDefault)
    , m_hasViewportUnits(false)
    , m_styleRecalcElementCounter(0)
    , m_frameView(nullptr)
{
    // We depend on the url getting immediately set in subframes, but we
    // also depend on the url NOT getting immediately set in opened windows.
    // See fast/dom/early-frame-url.html
    // and fast/dom/location-new-window-no-crash.html, respectively.
    // FIXME: Can/should we unify this behavior?
    if (initializer.shouldSetURL())
        setURL(initializer.url());

    InspectorCounters::incrementCounter(InspectorCounters::DocumentCounter);

#ifndef NDEBUG
    liveDocumentSet().add(this);
#endif
}

Document::~Document()
{
    ASSERT(!renderView());
    ASSERT(!parentTreeScope());
    ASSERT(m_ranges.isEmpty());
    ASSERT(!hasGuardRefCount());

    if (m_elemSheet)
        m_elemSheet->clearOwnerNode();

    // We must call clearRareData() here since a Document class inherits TreeScope
    // as well as Node. See a comment on TreeScope.h for the reason.
    if (hasRareData())
        clearRareData();

#ifndef NDEBUG
    liveDocumentSet().remove(this);
#endif

    InspectorCounters::decrementCounter(InspectorCounters::DocumentCounter);
}

#if !ENABLE(OILPAN)
void Document::dispose()
{
    ASSERT_WITH_SECURITY_IMPLICATION(!m_deletionHasBegun);

    // We must make sure not to be retaining any of our children through
    // these extra pointers or we will create a reference cycle.
    m_hoverNode = nullptr;
    m_activeHoverElement = nullptr;
    m_userActionElements.documentDidRemoveLastRef();

    detachParser();

    // removeDetachedChildren() doesn't always unregister IDs,
    // so tear down scope information upfront to avoid having stale references in the map.
    destroyTreeScopeData();

    removeDetachedChildren();

    m_markers->clear();

    if (m_scriptedAnimationController)
        m_scriptedAnimationController->clearDocumentPointer();
    m_scriptedAnimationController.clear();
}
#endif

SelectorQueryCache& Document::selectorQueryCache()
{
    if (!m_selectorQueryCache)
        m_selectorQueryCache = adoptPtr(new SelectorQueryCache());
    return *m_selectorQueryCache;
}

MediaQueryMatcher& Document::mediaQueryMatcher()
{
    if (!m_mediaQueryMatcher)
        m_mediaQueryMatcher = MediaQueryMatcher::create(*this);
    return *m_mediaQueryMatcher;
}

void Document::mediaQueryAffectingValueChanged()
{
    m_evaluateMediaQueriesOnStyleRecalc = true;
    // FIXME(sky): Actually update media queries from <style media>.
}

PassRefPtr<Element> Document::createElement(const AtomicString& name, ExceptionState& exceptionState)
{
    if (!isValidName(name)) {
        exceptionState.ThrowDOMException(InvalidCharacterError, "The tag name provided ('" + name + "') is not a valid name.");
        return nullptr;
    }

    return HTMLElement::create(QualifiedName(name), *this);
}

PassRefPtr<Text> Document::createText(const String& text)
{
    return Text::create(*this, text);
}

PassRefPtr<DocumentFragment> Document::createDocumentFragment()
{
    return DocumentFragment::create(*this);
}

PassRefPtr<Text> Document::createEditingTextNode(const String& text)
{
    return Text::createEditingText(*this, text);
}

bool Document::importContainerNodeChildren(ContainerNode* oldContainerNode, PassRefPtr<ContainerNode> newContainerNode, ExceptionState& exceptionState)
{
    for (Node* oldChild = oldContainerNode->firstChild(); oldChild; oldChild = oldChild->nextSibling()) {
        RefPtr<Node> newChild = importNode(oldChild, true, exceptionState);
        if (exceptionState.had_exception())
            return false;
        newContainerNode->appendChild(newChild.release(), exceptionState);
        if (exceptionState.had_exception())
            return false;
    }

    return true;
}

PassRefPtr<Node> Document::importNode(Node* importedNode, bool deep, ExceptionState& exceptionState)
{
    switch (importedNode->nodeType()) {
    case TEXT_NODE:
        return Text::create(*this, toText(importedNode)->data());
    case ELEMENT_NODE: {
        Element* oldElement = toElement(importedNode);
        RefPtr<Element> newElement = createElement(oldElement->tagQName(), false);

        newElement->cloneDataFromElement(*oldElement);

        if (deep) {
            if (!importContainerNodeChildren(oldElement, newElement, exceptionState))
                return nullptr;
        }

        return newElement.release();
    }
    case DOCUMENT_FRAGMENT_NODE: {
        DocumentFragment* oldFragment = toDocumentFragment(importedNode);
        RefPtr<DocumentFragment> newFragment = createDocumentFragment();
        if (deep && !importContainerNodeChildren(oldFragment, newFragment, exceptionState))
            return nullptr;

        return newFragment.release();
    }
    case DOCUMENT_NODE:
        exceptionState.ThrowDOMException(NotSupportedError, "The node provided is a document, which may not be imported.");
        return nullptr;
    }

    ASSERT_NOT_REACHED();
    return nullptr;
}

PassRefPtr<Node> Document::adoptNode(PassRefPtr<Node> source, ExceptionState& exceptionState)
{
    switch (source->nodeType()) {
    case DOCUMENT_NODE:
        exceptionState.ThrowDOMException(NotSupportedError, "The node provided is of type '" + source->nodeName() + "', which may not be adopted.");
        return nullptr;
    default:
        if (source->parentNode()) {
            source->parentNode()->removeChild(source.get(), exceptionState);
            if (exceptionState.had_exception())
                return nullptr;
        }
    }

    this->adoptIfNeeded(*source);

    return source;
}

PassRefPtr<Element> Document::createElement(const QualifiedName& qName, bool createdByParser)
{
    return HTMLElement::create(qName, *this);
}

String Document::readyState() const
{
    DEFINE_STATIC_LOCAL(const String, loading, ("loading"));
    DEFINE_STATIC_LOCAL(const String, interactive, ("interactive"));
    DEFINE_STATIC_LOCAL(const String, complete, ("complete"));

    switch (m_readyState) {
    case Loading:
        return loading;
    case Interactive:
        return interactive;
    case Complete:
        return complete;
    }

    ASSERT_NOT_REACHED();
    return String();
}

void Document::setReadyState(ReadyState readyState)
{
    if (readyState == m_readyState)
        return;
    m_readyState = readyState;
}

bool Document::isLoadCompleted()
{
    return m_readyState == Complete;
}

AtomicString Document::encodingName() const
{
    // TextEncoding::name() returns a char*, no need to allocate a new
    // String for it each time.
    // FIXME: We should fix TextEncoding to speak AtomicString anyway.
    return AtomicString(encoding().name());
}

void Document::setContentLanguage(const AtomicString& language)
{
    if (m_contentLanguage == language)
        return;
    m_contentLanguage = language;

    // Document's style depends on the content language.
    setNeedsStyleRecalc(SubtreeStyleChange);
}

KURL Document::baseURI() const
{
    return m_baseURL;
}

AtomicString Document::contentType() const
{
    return AtomicString("text/html");
}

Element* Document::elementFromPoint(int x, int y) const
{
    if (!renderView())
        return 0;

    return TreeScope::elementFromPoint(x, y);
}

PassRefPtr<Range> Document::caretRangeFromPoint(int x, int y)
{
    if (!renderView())
        return nullptr;
    HitTestResult result = hitTestInDocument(this, x, y);
    RenderObject* renderer = result.renderer();
    if (!renderer)
        return nullptr;

    PositionWithAffinity positionWithAffinity = result.position();
    if (positionWithAffinity.position().isNull())
        return nullptr;

    Position rangeCompliantPosition = positionWithAffinity.position().parentAnchoredEquivalent();
    return Range::create(*this, rangeCompliantPosition, rangeCompliantPosition);
}

/*
 * Performs three operations:
 *  1. Convert control characters to spaces
 *  2. Trim leading and trailing spaces
 *  3. Collapse internal whitespace.
 */
template <typename CharacterType>
static inline String canonicalizedTitle(Document* document, const String& title)
{
    const CharacterType* characters = title.getCharacters<CharacterType>();
    unsigned length = title.length();
    unsigned i;

    StringBuffer<CharacterType> buffer(length);
    unsigned builderIndex = 0;

    // Skip leading spaces and leading characters that would convert to spaces
    for (i = 0; i < length; ++i) {
        CharacterType c = characters[i];
        if (!(c <= 0x20 || c == 0x7F))
            break;
    }

    if (i == length)
        return String();

    // Replace control characters with spaces, and backslashes with currency symbols, and collapse whitespace.
    bool previousCharWasWS = false;
    for (; i < length; ++i) {
        CharacterType c = characters[i];
        if (c <= 0x20 || c == 0x7F || (WTF::Unicode::category(c) & (WTF::Unicode::Separator_Line | WTF::Unicode::Separator_Paragraph))) {
            if (previousCharWasWS)
                continue;
            buffer[builderIndex++] = ' ';
            previousCharWasWS = true;
        } else {
            buffer[builderIndex++] = c;
            previousCharWasWS = false;
        }
    }

    // Strip trailing spaces
    while (builderIndex > 0) {
        --builderIndex;
        if (buffer[builderIndex] != ' ')
            break;
    }

    if (!builderIndex && buffer[builderIndex] == ' ')
        return String();

    buffer.shrink(builderIndex + 1);

    return String::adopt(buffer);
}

const AtomicString& Document::dir()
{
    return nullAtom;
}

void Document::setDir(const AtomicString& value)
{
}

String Document::nodeName() const
{
    return "#document";
}

Node::NodeType Document::nodeType() const
{
    return DOCUMENT_NODE;
}

void Document::setStateForNewFormElements(const Vector<String>& stateVector)
{
}

FrameView* Document::view() const
{
    if (m_frameView)
        return m_frameView;
    return m_frame ? m_frame->view() : 0;
}

Page* Document::page() const
{
    return m_frame ? m_frame->page() : 0;
}

FrameHost* Document::frameHost() const
{
    return m_frame ? m_frame->host() : 0;
}

Settings* Document::settings() const
{
    return m_frame ? m_frame->settings() : 0;
}

PassRefPtr<Range> Document::createRange()
{
    return Range::create(*this);
}

bool Document::needsRenderTreeUpdate() const
{
    if (!isActive() || !view())
        return false;
    if (needsFullRenderTreeUpdate())
        return true;
    if (childNeedsStyleRecalc())
        return true;
    return false;
}

bool Document::needsFullRenderTreeUpdate() const
{
    if (!isActive() || !view())
        return false;
    if (needsStyleRecalc())
        return true;
    return false;
}

bool Document::shouldScheduleRenderTreeUpdate() const
{
    if (!isActive())
        return false;
    if (inStyleRecalc())
        return false;
    return true;
}

void Document::scheduleRenderTreeUpdate()
{
    ASSERT(!hasPendingStyleRecalc());
    ASSERT(shouldScheduleRenderTreeUpdate());
    ASSERT(needsRenderTreeUpdate());

    scheduleVisualUpdate();
    m_visualUpdatePending = true;
}

void Document::scheduleVisualUpdate()
{
}

void Document::setupFontBuilder(RenderStyle* documentStyle)
{
    FontBuilder fontBuilder;
    fontBuilder.initForStyleResolve(*this, documentStyle);
    RefPtr<CSSFontSelector> selector = m_styleEngine->fontSelector();
    fontBuilder.createFontForDocument(selector, documentStyle);
}

void Document::updateRenderTree(StyleRecalcChange change)
{
    ASSERT(isMainThread());

    ScriptForbiddenScope forbidScript;

    if (!view() || !isActive())
        return;

    if (change != Force && !needsRenderTreeUpdate())
        return;

    if (inStyleRecalc())
        return;

    // Entering here from inside layout or paint would be catastrophic since recalcStyle can
    // tear down the render tree or (unfortunately) run script. Kill the whole renderer if
    // someone managed to get into here from inside layout or paint.
    RELEASE_ASSERT(!view()->isInPerformLayout());
    RELEASE_ASSERT(!view()->isPainting());

    // Script can run below in WidgetUpdates, so protect the LocalFrame.
    // FIXME: Can this still happen? How does script run inside
    // UpdateSuspendScope::performDeferredWidgetTreeOperations() ?
    RefPtr<LocalFrame> protect(m_frame);

    TRACE_EVENT0("blink", "Document::updateRenderTree");

    m_styleRecalcElementCounter = 0;

    evaluateMediaQueryListIfNeeded();

    // FIXME: We should update style on our ancestor chain before proceeding
    // however doing so currently causes several tests to crash, as LocalFrame::setDocument calls Document::attach
    // before setting the LocalDOMWindow on the LocalFrame, or the SecurityOrigin on the document. The attach, in turn
    // resolves style (here) and then when we resolve style on the parent chain, we may end up
    // re-attaching our containing iframe, which when asked HTMLFrameElementBase::isURLAllowed
    // hits a null-dereference due to security code always assuming the document has a SecurityOrigin.

    updateStyle(change);
}

void Document::updateStyle(StyleRecalcChange change)
{
    TRACE_EVENT0("blink", "Document::updateStyle");

    m_inStyleRecalc = true;

    if (styleChangeType() >= SubtreeStyleChange)
        change = Force;

    // FIXME: Cannot access the styleResolver() before calling styleForDocument below because
    // apparently the StyleResolver's constructor has side effects. We should fix it.
    // See printing/setPrinting.html, printing/width-overflow.html though they only fail on
    // mac when accessing the resolver by what appears to be a viewport size difference.

    if (change == Force) {
        RefPtr<RenderStyle> documentStyle = StyleResolver::styleForDocument(*this);
        StyleRecalcChange localChange = RenderStyle::stylePropagationDiff(documentStyle.get(), renderView()->style());
        if (localChange != NoChange)
            renderView()->setStyle(documentStyle.release());
    }

    clearNeedsStyleRecalc();

    // Uncomment to enable printing of statistics about style sharing and the matched property cache.
    // Optionally pass StyleResolver::ReportSlowStats to print numbers that require crawling the
    // entire DOM (where collecting them is very slow).
    // FIXME: Expose this as a runtime flag.
    // styleResolver().enableStats(/*StyleResolver::ReportSlowStats*/);

    if (StyleResolverStats* stats = styleResolver().stats())
        stats->reset();

    for (Element* element = ElementTraversal::firstChild(*this); element; element = ElementTraversal::nextSibling(*element)) {
        if (element->shouldCallRecalcStyle(change))
            element->recalcStyle(change);
    }

    styleResolver().printStats();

    view()->recalcOverflowAfterStyleChange();

    clearChildNeedsStyleRecalc();

    m_styleEngine->resolver().clearStyleSharingList();

    m_visualUpdatePending = false;
    m_inStyleRecalc = false;

    ASSERT(!needsStyleRecalc());
    ASSERT(!childNeedsStyleRecalc());
}

void Document::updateRenderTreeForNodeIfNeeded(Node* node)
{
    bool needsRecalc = needsFullRenderTreeUpdate();

    for (const Node* ancestor = node; ancestor && !needsRecalc; ancestor = ancestor->parentNode())
        needsRecalc = ancestor->needsStyleRecalc();

    if (needsRecalc)
        updateRenderTreeIfNeeded();
}

void Document::updateLayout()
{
    ASSERT(isMainThread());

    ScriptForbiddenScope forbidScript;

    RefPtr<FrameView> frameView = view();
    if (frameView && frameView->isInPerformLayout()) {
        // View layout should not be re-entrant.
        ASSERT_NOT_REACHED();
        return;
    }

    updateRenderTreeIfNeeded();

    if (!isActive())
        return;

    if (frameView->needsLayout())
        frameView->layout();
}

StyleResolver& Document::styleResolver() const
{
    ASSERT(isActive());
    return m_styleEngine->resolver();
}

void Document::attach(const AttachContext& context)
{
    m_styleEngine = StyleEngine::create(*this);

    m_renderView = new RenderView(this);
    setRenderer(m_renderView);

    m_renderView->setStyle(StyleResolver::styleForDocument(*this));

    ContainerNode::attach(context);

    m_active = true;
}

void Document::detach(const AttachContext& context)
{
    ASSERT(isActive());

    m_active = false;
    m_stopped = true;

    if (page())
        page()->documentDetached(this);

    if (m_scriptedAnimationController)
        m_scriptedAnimationController->clearDocumentPointer();
    m_scriptedAnimationController.clear();

    m_hoverNode = nullptr;
    m_activeHoverElement = nullptr;

    m_renderView = 0;
    ContainerNode::detach(context);

    m_styleEngine = nullptr;

    // This is required, as our LocalFrame might delete itself as soon as it detaches
    // us. However, this violates Node::detach() semantics, as it's never
    // possible to re-attach. Eventually Document::detach() should be renamed,
    // or this setting of the frame to 0 could be made explicit in each of the
    // callers of Document::detach().
    m_frame = 0;

    if (m_mediaQueryMatcher)
        m_mediaQueryMatcher->documentDetached();
}

void Document::prepareForDestruction()
{
    m_markers->prepareForDestruction();

    // The process of disconnecting descendant frames could have already detached us.
    if (!isActive())
        return;

    if (LocalDOMWindow* window = this->domWindow())
        window->willDetachDocumentFromFrame();
    detach();
}

void Document::detachParser()
{
}

void Document::cancelParsing()
{
}

DocumentParser* Document::startParsing()
{
    return nullptr;
}

void Document::implicitClose()
{
    ASSERT(!inStyleRecalc());

    bool doload = false;

    // If the load was blocked because of a pending location change and the location change triggers a same document
    // navigation, don't fire load events after the same document navigation completes (unless there's an explicit open).
    m_loadEventProgress = LoadEventTried;

    if (!doload)
        return;

    // The call to dispatchWindowLoadEvent can detach the LocalDOMWindow and cause it (and its
    // attached Document) to be destroyed.
    RefPtr<LocalDOMWindow> protectedWindow(this->domWindow());

    m_loadEventProgress = LoadEventInProgress;

    // We have to clear the parser, in case someone document.write()s from the
    // onLoad event handler, as in Radar 3206524.
    detachParser();

    if (frame())
        frame()->loaderClient()->dispatchDidHandleOnloadEvents();

    if (!frame()) {
        m_loadEventProgress = LoadEventCompleted;
        return;
    }

    // FIXME(sky): Could start frame if they are still paused here.
    m_loadEventProgress = LoadEventCompleted;
}

void Document::checkCompleted()
{
    if (isLoadCompleted())
        return;

    // Are we still parsing?
    if (parsing())
        return;

    // Still waiting for elements that don't go through a FrameLoader?
    if (isDelayingLoadEvent())
        return;

    // OK, completed.
    setReadyState(Complete);
    if (loadEventStillNeeded())
        implicitClose();
}

void Document::dispatchUnloadEvents()
{
}

Document::PageDismissalType Document::pageDismissalEventBeingDispatched() const
{
    if (m_loadEventProgress == PageHideInProgress)
        return PageHideDismissal;
    if (m_loadEventProgress == UnloadEventInProgress)
        return UnloadDismissal;
    return NoDismissal;
}

void Document::setParsing(bool b)
{
    m_isParsing = b;

    if (m_isParsing && !m_elementDataCache)
        m_elementDataCache = ElementDataCache::create();
}

int Document::elapsedTime() const
{
    return static_cast<int>((currentTime() - m_startTime) * 1000);
}

const KURL& Document::virtualURL() const
{
    return m_url;
}

KURL Document::virtualCompleteURL(const String& url) const
{
    return completeURL(url);
}

void Document::setURL(const KURL& url)
{
    const KURL& newURL = url.isEmpty() ? blankURL() : url;
    if (newURL == m_url)
        return;

    m_url = newURL;
    updateBaseURL();
}

void Document::updateBaseURL()
{
    KURL oldBaseURL = m_baseURL;
    m_baseURL = m_url;

    if (!m_baseURL.isValid())
        m_baseURL = KURL();
}

void Document::didLoadAllParserBlockingResources()
{
}

TextPosition Document::parserPosition() const
{
    return TextPosition::belowRangePosition();
}

CSSStyleSheet& Document::elementSheet()
{
    if (!m_elemSheet)
        m_elemSheet = CSSStyleSheet::create(this, m_baseURL);
    return *m_elemSheet;
}

String Document::outgoingReferrer()
{
    // See http://www.whatwg.org/specs/web-apps/current-work/#fetching-resources
    // for why we walk the parent chain for srcdoc documents.
    Document* referrerDocument = this;
    return referrerDocument->m_url.strippedForUseAsReferrer();
}

PassRefPtr<Node> Document::cloneNode(bool deep)
{
    RefPtr<Document> clone = cloneDocumentWithoutChildren();
    if (deep)
        cloneChildNodes(clone.get());
    return clone.release();
}

PassRefPtr<Document> Document::cloneDocumentWithoutChildren()
{
    return create(DocumentInit(url()));
}

void Document::evaluateMediaQueryListIfNeeded()
{
    if (!m_evaluateMediaQueriesOnStyleRecalc)
        return;
    evaluateMediaQueryList();
    m_evaluateMediaQueriesOnStyleRecalc = false;
}

void Document::evaluateMediaQueryList()
{
}

void Document::notifyResizeForViewportUnits()
{
    if (!hasViewportUnits())
        return;
    styleResolver().notifyResizeForViewportUnits();
    setNeedsStyleRecalcForViewportUnits();
}

void Document::styleResolverChanged()
{
    // styleResolverChanged() can be invoked during Document destruction.
    // We just skip that case.
    if (!m_styleEngine)
        return;
    m_styleEngine->resolverChanged();
}

void Document::setHoverNode(PassRefPtr<Node> newHoverNode)
{
    m_hoverNode = newHoverNode;
}

void Document::setActiveHoverElement(PassRefPtr<Element> newActiveElement)
{
    if (!newActiveElement) {
        m_activeHoverElement.clear();
        return;
    }

    m_activeHoverElement = newActiveElement;
}

void Document::hoveredNodeDetached(Node* node)
{
    if (!m_hoverNode)
        return;

    if (node != m_hoverNode && (!m_hoverNode->isTextNode() || node != m_hoverNode.get()->parentNode()))
        return;

    m_hoverNode = node->parentNode();
    while (m_hoverNode && !m_hoverNode->renderer())
        m_hoverNode = m_hoverNode.get()->parentNode();
}

void Document::activeChainNodeDetached(Node* node)
{
    if (!m_activeHoverElement)
        return;

    if (node != m_activeHoverElement)
        return;

    Node* activeNode = node->parentNode();
    while (activeNode && activeNode->isElementNode() && !activeNode->renderer())
        activeNode = activeNode->parentNode();

    m_activeHoverElement = activeNode && activeNode->isElementNode() ? toElement(activeNode) : 0;
}

void Document::updateRangesAfterChildrenChanged(ContainerNode* container)
{
    if (!m_ranges.isEmpty()) {
        AttachedRangeSet::const_iterator end = m_ranges.end();
        for (AttachedRangeSet::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->nodeChildrenChanged(container);
    }
}

void Document::updateRangesAfterNodeMovedToAnotherDocument(const Node& node)
{
    ASSERT(node.document() != this);
    if (m_ranges.isEmpty())
        return;
    AttachedRangeSet ranges = m_ranges;
    AttachedRangeSet::const_iterator end = ranges.end();
    for (AttachedRangeSet::const_iterator it = ranges.begin(); it != end; ++it)
        (*it)->updateOwnerDocumentIfNeeded();
}

void Document::nodeChildrenWillBeRemoved(ContainerNode& container)
{
    EventDispatchForbiddenScope assertNoEventDispatch;
    if (!m_ranges.isEmpty()) {
        AttachedRangeSet::const_iterator end = m_ranges.end();
        for (AttachedRangeSet::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->nodeChildrenWillBeRemoved(container);
    }
}

void Document::nodeWillBeRemoved(Node& n)
{
    if (!m_ranges.isEmpty()) {
        AttachedRangeSet::const_iterator rangesEnd = m_ranges.end();
        for (AttachedRangeSet::const_iterator it = m_ranges.begin(); it != rangesEnd; ++it)
            (*it)->nodeWillBeRemoved(n);
    }
}

void Document::didInsertText(Node* text, unsigned offset, unsigned length)
{
    if (!m_ranges.isEmpty()) {
        AttachedRangeSet::const_iterator end = m_ranges.end();
        for (AttachedRangeSet::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->didInsertText(text, offset, length);
    }

    // Update the markers for spelling and grammar checking.
    m_markers->shiftMarkers(text, offset, length);
}

void Document::didRemoveText(Node* text, unsigned offset, unsigned length)
{
    if (!m_ranges.isEmpty()) {
        AttachedRangeSet::const_iterator end = m_ranges.end();
        for (AttachedRangeSet::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->didRemoveText(text, offset, length);
    }

    // Update the markers for spelling and grammar checking.
    m_markers->removeMarkers(text, offset, length);
    m_markers->shiftMarkers(text, offset + length, 0 - length);
}

void Document::didMergeTextNodes(Text& oldNode, unsigned offset)
{
    if (!m_ranges.isEmpty()) {
        NodeWithIndex oldNodeWithIndex(oldNode);
        AttachedRangeSet::const_iterator end = m_ranges.end();
        for (AttachedRangeSet::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->didMergeTextNodes(oldNodeWithIndex, offset);
    }

    // FIXME: This should update markers for spelling and grammar checking.
}

void Document::didSplitTextNode(Text& oldNode)
{
    if (!m_ranges.isEmpty()) {
        AttachedRangeSet::const_iterator end = m_ranges.end();
        for (AttachedRangeSet::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->didSplitTextNode(oldNode);
    }

    // FIXME: This should update markers for spelling and grammar checking.
}

const AtomicString& Document::referrer() const
{
    return nullAtom;
}

static bool isValidNameNonASCII(const LChar* characters, unsigned length)
{
    if (!isValidNameStart(characters[0]))
        return false;

    for (unsigned i = 1; i < length; ++i) {
        if (!isValidNamePart(characters[i]))
            return false;
    }

    return true;
}

static bool isValidNameNonASCII(const UChar* characters, unsigned length)
{
    unsigned i = 0;

    UChar32 c;
    U16_NEXT(characters, i, length, c)
    if (!isValidNameStart(c))
        return false;

    while (i < length) {
        U16_NEXT(characters, i, length, c)
        if (!isValidNamePart(c))
            return false;
    }

    return true;
}

template<typename CharType>
static inline bool isValidNameASCII(const CharType* characters, unsigned length)
{
    CharType c = characters[0];
    if (!(isASCIIAlpha(c) || c == ':' || c == '_'))
        return false;

    for (unsigned i = 1; i < length; ++i) {
        c = characters[i];
        if (!(isASCIIAlphanumeric(c) || c == ':' || c == '_' || c == '-' || c == '.'))
            return false;
    }

    return true;
}

bool Document::isValidName(const String& name)
{
    unsigned length = name.length();
    if (!length)
        return false;

    if (name.is8Bit()) {
        const LChar* characters = name.characters8();

        if (isValidNameASCII(characters, length))
            return true;

        return isValidNameNonASCII(characters, length);
    }

    const UChar* characters = name.characters16();

    if (isValidNameASCII(characters, length))
        return true;

    return isValidNameNonASCII(characters, length);
}

template<typename CharType>
static bool parseQualifiedNameInternal(const AtomicString& qualifiedName, const CharType* characters, unsigned length, AtomicString& prefix, AtomicString& localName, ExceptionState& exceptionState)
{
    bool nameStart = true;
    bool sawColon = false;
    int colonPos = 0;

    for (unsigned i = 0; i < length;) {
        UChar32 c;
        U16_NEXT(characters, i, length, c)
        if (c == ':') {
            if (sawColon) {
                exceptionState.ThrowDOMException(NamespaceError, "The qualified name provided ('" + qualifiedName + "') contains multiple colons.");
                return false; // multiple colons: not allowed
            }
            nameStart = true;
            sawColon = true;
            colonPos = i - 1;
        } else if (nameStart) {
            if (!isValidNameStart(c)) {
                StringBuilder message;
                message.appendLiteral("The qualified name provided ('");
                message.append(qualifiedName);
                message.appendLiteral("') contains the invalid name-start character '");
                message.append(c);
                message.appendLiteral("'.");
                exceptionState.ThrowDOMException(InvalidCharacterError, message.toString());
                return false;
            }
            nameStart = false;
        } else {
            if (!isValidNamePart(c)) {
                StringBuilder message;
                message.appendLiteral("The qualified name provided ('");
                message.append(qualifiedName);
                message.appendLiteral("') contains the invalid character '");
                message.append(c);
                message.appendLiteral("'.");
                exceptionState.ThrowDOMException(InvalidCharacterError, message.toString());
                return false;
            }
        }
    }

    if (!sawColon) {
        prefix = nullAtom;
        localName = qualifiedName;
    } else {
        prefix = AtomicString(characters, colonPos);
        if (prefix.isEmpty()) {
            exceptionState.ThrowDOMException(NamespaceError, "The qualified name provided ('" + qualifiedName + "') has an empty namespace prefix.");
            return false;
        }
        int prefixStart = colonPos + 1;
        localName = AtomicString(characters + prefixStart, length - prefixStart);
    }

    if (localName.isEmpty()) {
        exceptionState.ThrowDOMException(NamespaceError, "The qualified name provided ('" + qualifiedName + "') has an empty local name.");
        return false;
    }

    return true;
}

bool Document::parseQualifiedName(const AtomicString& qualifiedName, AtomicString& prefix, AtomicString& localName, ExceptionState& exceptionState)
{
    unsigned length = qualifiedName.length();

    if (!length) {
        exceptionState.ThrowDOMException(InvalidCharacterError, "The qualified name provided is empty.");
        return false;
    }

    if (qualifiedName.is8Bit())
        return parseQualifiedNameInternal(qualifiedName, qualifiedName.characters8(), length, prefix, localName, exceptionState);
    return parseQualifiedNameInternal(qualifiedName, qualifiedName.characters16(), length, prefix, localName, exceptionState);
}

KURL Document::completeURL(const String& url) const
{
    // Always return a null URL when passed a null string.
    // FIXME: Should we change the KURL constructor to have this behavior?
    // See also [CSS]StyleSheet::completeURL(const String&)
    if (url.isNull())
        return KURL();
    return KURL(m_baseURL, url);
}

KURL Document::openSearchDescriptionURL()
{
    return KURL();
}

Document& Document::topDocument() const
{
    Document* doc = const_cast<Document*>(this);
    ASSERT(doc);
    return *doc;
}

WeakPtr<Document> Document::contextDocument()
{
    return WeakPtr<Document>(nullptr);
}

void Document::finishedParsing()
{
}

bool Document::allowExecutingScripts(Node* node)
{
    return false;
}

void Document::attachRange(Range* range)
{
    ASSERT(!m_ranges.contains(range));
    m_ranges.add(range);
}

void Document::detachRange(Range* range)
{
    // We don't ASSERT m_ranges.contains(range) to allow us to call this
    // unconditionally to fix: https://bugs.webkit.org/show_bug.cgi?id=26044
    m_ranges.remove(range);
}

void Document::reportBlockedScriptExecutionToInspector(const String& directiveText)
{
}

void Document::decrementLoadEventDelayCount()
{
}

void Document::checkLoadEventSoon()
{
}

bool Document::isDelayingLoadEvent()
{
    return false;
}


ScriptedAnimationController& Document::ensureScriptedAnimationController()
{
    if (!m_scriptedAnimationController) {
        m_scriptedAnimationController = ScriptedAnimationController::create(this);
        // We need to make sure that we don't start up the animation controller on a background tab, for example.
        if (!page())
            m_scriptedAnimationController->suspend();
    }
    return *m_scriptedAnimationController;
}

int Document::requestAnimationFrame(PassOwnPtr<RequestAnimationFrameCallback> callback)
{
    return ensureScriptedAnimationController().registerCallback(callback);
}

void Document::cancelAnimationFrame(int id)
{
    if (!m_scriptedAnimationController)
        return;
    m_scriptedAnimationController->cancelCallback(id);
}

void Document::serviceScriptedAnimations(double monotonicAnimationStartTime)
{
    WTF_LOG(ScriptedAnimationController, "Document::serviceScriptedAnimations: controller = %d",
        m_scriptedAnimationController ? 1 : 0);
    if (!m_scriptedAnimationController)
        return;
    m_scriptedAnimationController->serviceScriptedAnimations(monotonicAnimationStartTime);
}

DocumentLoadTiming* Document::timing() const
{
    return &m_documentLoadTiming;
}

IntSize Document::initialViewportSize() const
{
    if (!view())
        return IntSize();
    return view()->unscaledVisibleContentSize();
}

void Document::decrementActiveParserCount()
{
    --m_activeParserCount;
}

float Document::devicePixelRatio() const
{
    return m_frame ? m_frame->devicePixelRatio() : 1.0;
}

Picture* Document::rootPicture() const
{
    return m_picture.get();
}

void Document::setRootPicture(PassRefPtr<Picture> picture)
{
    m_picture = picture;
    scheduleVisualUpdate();
}

} // namespace blink

#ifndef NDEBUG
using namespace blink;
void showLiveDocumentInstances()
{
    WeakDocumentSet& set = liveDocumentSet();
    fprintf(stderr, "There are %u documents currently alive:\n", set.size());
    for (WeakDocumentSet::const_iterator it = set.begin(); it != set.end(); ++it) {
        fprintf(stderr, "- Document %p URL: %s\n", *it, (*it)->url().string().utf8().data());
    }
}
#endif
