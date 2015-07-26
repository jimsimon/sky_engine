/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef SKY_ENGINE_CORE_DOM_DOCUMENT_H_
#define SKY_ENGINE_CORE_DOM_DOCUMENT_H_

#include "sky/engine/bindings/exception_state_placeholder.h"
#include "sky/engine/core/dom/ContainerNode.h"
#include "sky/engine/core/dom/DocumentInit.h"
#include "sky/engine/core/dom/DocumentSupplementable.h"
#include "sky/engine/core/dom/MutationObserver.h"
#include "sky/engine/core/dom/TextLinkColors.h"
#include "sky/engine/core/dom/TreeScope.h"
#include "sky/engine/core/dom/UserActionElementSet.h"
#include "sky/engine/core/inspector/ScriptCallStack.h"
#include "sky/engine/core/loader/DocumentLoadTiming.h"
#include "sky/engine/platform/Length.h"
#include "sky/engine/platform/heap/Handle.h"
#include "sky/engine/platform/weborigin/KURL.h"
#include "sky/engine/platform/weborigin/ReferrerPolicy.h"
#include "sky/engine/tonic/dart_value.h"
#include "sky/engine/wtf/HashSet.h"
#include "sky/engine/wtf/OwnPtr.h"
#include "sky/engine/wtf/PassOwnPtr.h"
#include "sky/engine/wtf/PassRefPtr.h"
#include "sky/engine/wtf/WeakPtr.h"
#include "sky/engine/wtf/text/TextEncoding.h"
#include "sky/engine/wtf/text/TextPosition.h"

namespace blink {

class AbstractModule;
class Attr;
class Comment;
class ConsoleMessage;
class CSSStyleDeclaration;
class CSSStyleSheet;
class DocumentFragment;
class DocumentLoadTiming;
class DocumentMarkerController;
class DocumentParser;
class Element;
class ElementDataCache;
class Event;
class ExceptionState;
class FloatQuad;
class FloatRect;
class Frame;
class FrameHost;
class FrameView;
class HitTestRequest;
class HTMLElement;
class LayoutPoint;
class LocalDOMWindow;
class LocalFrame;
class MediaQueryListListener;
class MediaQueryMatcher;
class Page;
class Picture;
class QualifiedName;
class Range;
class RenderView;
class RequestAnimationFrameCallback;
class ResourceFetcher;
class ScriptedAnimationController;
class ScriptRunner;
class SegmentedString;
class SelectorQueryCache;
class Settings;
class StyleEngine;
class StyleResolver;
class Text;

struct AnnotatedRegionValue;

typedef int ExceptionCode;

class Document;

class Document : public ContainerNode, public TreeScope, public DocumentSupplementable {
    DEFINE_WRAPPERTYPEINFO();
public:
    static PassRefPtr<Document> create(const DocumentInit& initializer = DocumentInit())
    {
        return adoptRef(new Document(initializer));
    }
    virtual ~Document();

    MediaQueryMatcher& mediaQueryMatcher();

    void mediaQueryAffectingValueChanged();

#if !ENABLE(OILPAN)
    using ContainerNode::ref;
    using ContainerNode::deref;
#endif
    using TreeScope::getElementById;

    virtual bool canContainRangeEndPoint() const override { return true; }

    SelectorQueryCache& selectorQueryCache();

    AbstractModule* module() const { return m_module; }
    void setModule(AbstractModule* module) { m_module = module; }

    // DOM methods & attributes for Document

    Length viewportDefaultMinWidth() const { return m_viewportDefaultMinWidth; }

    ReferrerPolicy referrerPolicy() const { return m_referrerPolicy; }

    String outgoingReferrer();

    PassRefPtr<Element> createElement(const AtomicString& name, ExceptionState&);
    PassRefPtr<Text> createText(const String& text);
    PassRefPtr<DocumentFragment> createDocumentFragment();
    PassRefPtr<Node> importNode(Node* importedNode, bool deep, ExceptionState&);
    PassRefPtr<Element> createElement(const QualifiedName&, bool createdByParser);

    Element* elementFromPoint(int x, int y) const;
    PassRefPtr<Range> caretRangeFromPoint(int x, int y);

    String readyState() const;

    AtomicString inputEncoding() const { return Document::encodingName(); }
    AtomicString charset() const { return Document::encodingName(); }
    AtomicString characterSet() const { return Document::encodingName(); }

    AtomicString encodingName() const;

    AtomicString contentType() const; // DOM 4 document.contentType

    const AtomicString& contentLanguage() const { return m_contentLanguage; }
    void setContentLanguage(const AtomicString&);

    KURL baseURI() const;

    PassRefPtr<Node> adoptNode(PassRefPtr<Node> source, ExceptionState&);

    StyleResolver& styleResolver() const;

    StyleEngine* styleEngine() { return m_styleEngine.get(); }

    // Called when one or more stylesheets in the document may have been added, removed, or changed.
    void styleResolverChanged();

    void evaluateMediaQueryList();

    void setStateForNewFormElements(const Vector<String>&);

    FrameView* view() const; // can be null
    LocalFrame* frame() const { return m_frame; } // can be null
    FrameHost* frameHost() const; // can be null
    Page* page() const; // can be null
    Settings* settings() const; // can be null

    float devicePixelRatio() const;

    PassRefPtr<Range> createRange();

    // Special support for editing
    PassRefPtr<Text> createEditingTextNode(const String&);

    void setupFontBuilder(RenderStyle* documentStyle);

    bool needsRenderTreeUpdate() const;
    void updateRenderTreeIfNeeded() { updateRenderTree(NoChange); }
    void updateRenderTreeForNodeIfNeeded(Node*);
    void updateLayout();

    void updateDistributionForNodeIfNeeded(Node*);

    virtual void attach(const AttachContext& = AttachContext()) override;
    virtual void detach(const AttachContext& = AttachContext()) override;
    void prepareForDestruction();

    // If you have a Document, use renderView() instead which is faster.
    void renderer() const = delete;

    RenderView* renderView() const { return m_renderView; }

    DocumentLoadTiming* timing() const;

    DocumentParser* startParsing();
    void cancelParsing();

    // close() is the DOM API document.close()
    void close(ExceptionState& = ASSERT_NO_EXCEPTION);
    // In some situations (see the code), we ignore document.close().
    // explicitClose() bypass these checks and actually tries to close the
    // input stream.
    void explicitClose();
    // implicitClose() actually does the work of closing the input stream.
    void implicitClose();
    void checkCompleted();

    void dispatchUnloadEvents();

    enum PageDismissalType {
        NoDismissal = 0,
        PageHideDismissal = 2,
        UnloadDismissal = 3
    };
    PageDismissalType pageDismissalEventBeingDispatched() const;

    const KURL& url() const { return m_url; }
    void setURL(const KURL&);

    // To understand how these concepts relate to one another, please see the
    // comments surrounding their declaration.
    const KURL& baseURL() const { return m_baseURL; }

    KURL completeURL(const String&) const;

    CSSStyleSheet& elementSheet();

    TextPosition parserPosition() const;

    enum ReadyState {
        Loading,
        Interactive,
        Complete
    };
    void setReadyState(ReadyState);
    bool isLoadCompleted();

    void setParsing(bool);
    bool parsing() const { return m_isParsing; }

    int elapsedTime() const;

    TextLinkColors& textLinkColors() { return m_textLinkColors; }

    UserActionElementSet& userActionElements()  { return m_userActionElements; }
    const UserActionElementSet& userActionElements() const { return m_userActionElements; }

    void setActiveHoverElement(PassRefPtr<Element>);
    Element* activeHoverElement() const { return m_activeHoverElement.get(); }

    void hoveredNodeDetached(Node*);
    void activeChainNodeDetached(Node*);

    void scheduleVisualUpdate();

    void scheduleRenderTreeUpdateIfNeeded();

    void attachRange(Range*);
    void detachRange(Range*);

    void updateRangesAfterChildrenChanged(ContainerNode*);
    void updateRangesAfterNodeMovedToAnotherDocument(const Node&);
    // nodeChildrenWillBeRemoved is used when removing all node children at once.
    void nodeChildrenWillBeRemoved(ContainerNode&);
    // nodeWillBeRemoved is only safe when removing one node at a time.
    void nodeWillBeRemoved(Node&);

    void didInsertText(Node*, unsigned offset, unsigned length);
    void didRemoveText(Node*, unsigned offset, unsigned length);
    void didMergeTextNodes(Text& oldNode, unsigned offset);
    void didSplitTextNode(Text& oldNode);

    void clearDOMWindow() { m_domWindow = nullptr; }
    LocalDOMWindow* domWindow() const { return m_domWindow; }

    // keep track of what types of event listeners are registered, so we don't
    // dispatch events unnecessarily
    enum ListenerType {
        DOMSUBTREEMODIFIED_LISTENER          = 1,
        DOMNODEINSERTED_LISTENER             = 1 << 1,
        DOMNODEREMOVED_LISTENER              = 1 << 2,
        DOMNODEREMOVEDFROMDOCUMENT_LISTENER  = 1 << 3,
        DOMNODEINSERTEDINTODOCUMENT_LISTENER = 1 << 4,
        DOMCHARACTERDATAMODIFIED_LISTENER    = 1 << 5,
    };

    bool hasListenerType(ListenerType listenerType) const { return (m_listenerTypes & listenerType); }
    void addListenerTypeIfNeeded(const AtomicString& eventType) { }

    bool hasMutationObserversOfType(MutationObserver::MutationType type) const
    {
        return m_mutationObserverTypes & type;
    }
    bool hasMutationObservers() const { return m_mutationObserverTypes; }
    void addMutationObserverTypes(MutationObserverOptions types) { m_mutationObserverTypes |= types; }

    const AtomicString& dir();
    void setDir(const AtomicString&);

    const AtomicString& referrer() const;

    String domain() const;
    void setDomain(const String& newDomain, ExceptionState&);

    // The following implements the rule from HTML 4 for what valid names are.
    // To get this right for all the XML cases, we probably have to improve this or move it
    // and make it sensitive to the type of document.
    static bool isValidName(const String&);

    // The following breaks a qualified name into a prefix and a local name.
    // It also does a validity check, and returns false if the qualified name
    // is invalid.  It also sets ExceptionCode when name is invalid.
    static bool parseQualifiedName(const AtomicString& qualifiedName, AtomicString& prefix, AtomicString& localName, ExceptionState&);

    DocumentMarkerController& markers() const { return *m_markers; }

    KURL openSearchDescriptionURL();

    Document& topDocument() const;
    WeakPtr<Document> contextDocument();

    void finishedParsing();

    const WTF::TextEncoding& encoding() const { return WTF::UTF8Encoding(); }

    bool allowExecutingScripts(Node*);

    enum LoadEventProgress {
        LoadEventNotRun,
        LoadEventTried,
        LoadEventInProgress,
        LoadEventCompleted,
        PageHideInProgress,
        UnloadEventInProgress,
        UnloadEventHandled
    };
    bool loadEventStillNeeded() const { return m_loadEventProgress == LoadEventNotRun; }
    bool processingLoadEvent() const { return m_loadEventProgress == LoadEventInProgress; }
    bool loadEventFinished() const { return m_loadEventProgress >= LoadEventCompleted; }
    bool unloadStarted() const { return m_loadEventProgress >= PageHideInProgress; }

    bool containsValidityStyleRules() const { return m_containsValidityStyleRules; }
    void setContainsValidityStyleRules() { m_containsValidityStyleRules = true; }

    void enqueueResizeEvent();
    void enqueueAnimationFrameEvent(PassRefPtr<Event>);
    // Only one event for a target/event type combination will be dispatched per frame.
    void enqueueUniqueAnimationFrameEvent(PassRefPtr<Event>);

    // Used to allow element that loads data without going through a FrameLoader to delay the 'load' event.
    void incrementLoadEventDelayCount() { }
    void decrementLoadEventDelayCount();
    void checkLoadEventSoon();
    bool isDelayingLoadEvent();

    int requestAnimationFrame(PassOwnPtr<RequestAnimationFrameCallback>);
    void cancelAnimationFrame(int id);
    void serviceScriptedAnimations(double monotonicAnimationStartTime);

    IntSize initialViewportSize() const;

    unsigned activeParserCount() { return m_activeParserCount; }
    void incrementActiveParserCount() { ++m_activeParserCount; }
    void decrementActiveParserCount();

    ElementDataCache* elementDataCache() { return m_elementDataCache.get(); }

    void didLoadAllParserBlockingResources();

    bool inStyleRecalc() const { return m_inStyleRecalc; }

    LocalFrame* executingFrame();

    bool isActive() const { return m_active; }
    bool isStopped() const { return m_stopped; }
    bool isDisposed() const { return m_stopped; }

    enum HttpRefreshType {
        HttpRefreshFromHeader,
        HttpRefreshFromMetaTag
    };
    void maybeHandleHttpRefresh(const String&, HttpRefreshType);

    void setHasViewportUnits() { m_hasViewportUnits = true; }
    bool hasViewportUnits() const { return m_hasViewportUnits; }
    void notifyResizeForViewportUnits();

    void didRecalculateStyleForElement() { ++m_styleRecalcElementCounter; }

    Picture* rootPicture() const;
    void setRootPicture(PassRefPtr<Picture> picture);

    void setFrame(LocalFrame* frame) { m_frame = frame; }
    void setFrameView(FrameView* view) { m_frameView = view; }

protected:
    explicit Document(const DocumentInit&);

#if !ENABLE(OILPAN)
    virtual void dispose() override;
#endif

    PassRefPtr<Document> cloneDocumentWithoutChildren();

    bool importContainerNodeChildren(ContainerNode* oldContainerNode, PassRefPtr<ContainerNode> newContainerNode, ExceptionState&);

private:
    friend class Node;

    bool isDocumentFragment() const = delete; // This will catch anyone doing an unnecessary check.
    bool isDocumentNode() const = delete; // This will catch anyone doing an unnecessary check.
    bool isElementNode() const = delete; // This will catch anyone doing an unnecessary check.

    ScriptedAnimationController& ensureScriptedAnimationController();

    bool hasPendingStyleRecalc() const { return m_visualUpdatePending; }

    bool shouldScheduleRenderTreeUpdate() const;
    void scheduleRenderTreeUpdate();

    bool needsFullRenderTreeUpdate() const;

    bool dirtyElementsForLayerUpdate();
    void updateDistributionIfNeeded();
    void evaluateMediaQueryListIfNeeded();

    void updateRenderTree(StyleRecalcChange);
    void updateStyle(StyleRecalcChange);

    void detachParser();

    virtual String nodeName() const override final;
    virtual NodeType nodeType() const override final;
    virtual PassRefPtr<Node> cloneNode(bool deep = true) override final;

    virtual const KURL& virtualURL() const final; // Same as url(), but without virtual call
    virtual KURL virtualCompleteURL(const String&) const final; // Same as completeURL() for the same reason as above.

    virtual void reportBlockedScriptExecutionToInspector(const String& directiveText) final;

    void updateBaseURL();

    void addListenerType(ListenerType listenerType) { m_listenerTypes |= listenerType; }

    void setHoverNode(PassRefPtr<Node>);
    Node* hoverNode() const { return m_hoverNode.get(); }

    bool m_active;
    bool m_visualUpdatePending;
    bool m_inStyleRecalc;
    bool m_stopped;

    AbstractModule* m_module;

    bool m_evaluateMediaQueriesOnStyleRecalc;

    LocalFrame* m_frame;
    RawPtr<LocalDOMWindow> m_domWindow;

    unsigned m_activeParserCount;

    // Document URLs.
    KURL m_url; // Document.URL: The URL from which this document was retrieved.
    KURL m_baseURL; // Node.baseURI: The URL to use when resolving relative URLs.

    // Mime-type of the document in case it was cloned or created by XHR.
    AtomicString m_mimeType;

    RefPtr<CSSStyleSheet> m_elemSheet;

    RefPtr<Node> m_hoverNode;
    RefPtr<Element> m_activeHoverElement;
    UserActionElementSet m_userActionElements;

    typedef HashSet<RawPtr<Range> > AttachedRangeSet;
    AttachedRangeSet m_ranges;

    unsigned short m_listenerTypes;

    MutationObserverOptions m_mutationObserverTypes;

    OwnPtr<StyleEngine> m_styleEngine;

    TextLinkColors m_textLinkColors;

    ReadyState m_readyState;
    bool m_isParsing;

    bool m_containsValidityStyleRules;

    OwnPtr<DocumentMarkerController> m_markers;

    LoadEventProgress m_loadEventProgress;

    double m_startTime;

    AtomicString m_contentLanguage;

    OwnPtr<SelectorQueryCache> m_selectorQueryCache;

    RenderView* m_renderView;

    WeakPtrFactory<Document> m_weakFactory;

    Length m_viewportDefaultMinWidth;

    bool m_didSetReferrerPolicy;
    ReferrerPolicy m_referrerPolicy;

    RefPtr<MediaQueryMatcher> m_mediaQueryMatcher;

    RefPtr<ScriptedAnimationController> m_scriptedAnimationController;

    OwnPtr<ElementDataCache> m_elementDataCache;

    bool m_hasViewportUnits;

    int m_styleRecalcElementCounter;
    mutable DocumentLoadTiming m_documentLoadTiming;

    RefPtr<Picture> m_picture;

    FrameView* m_frameView;
};

inline void Document::scheduleRenderTreeUpdateIfNeeded()
{
    // Inline early out to avoid the function calls below.
    if (hasPendingStyleRecalc())
        return;
    if (shouldScheduleRenderTreeUpdate() && needsRenderTreeUpdate())
        scheduleRenderTreeUpdate();
}

DEFINE_NODE_TYPE_CASTS(Document, isDocumentNode());

#define DEFINE_DOCUMENT_TYPE_CASTS(thisType) \
    DEFINE_TYPE_CASTS(thisType, Document, document, document->is##thisType(), document.is##thisType())

// This is needed to avoid ambiguous overloads with the Node and TreeScope versions.
DEFINE_COMPARISON_OPERATORS_WITH_REFERENCES(Document)

// Put these methods here, because they require the Document definition, but we really want to inline them.

inline bool Node::isDocumentNode() const
{
    return this == document();
}

} // namespace blink

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showLiveDocumentInstances();
#endif

#endif  // SKY_ENGINE_CORE_DOM_DOCUMENT_H_
