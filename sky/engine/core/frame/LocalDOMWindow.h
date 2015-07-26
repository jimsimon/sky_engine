/*
 * Copyright (C) 2006, 2007, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SKY_ENGINE_CORE_FRAME_LOCALDOMWINDOW_H_
#define SKY_ENGINE_CORE_FRAME_LOCALDOMWINDOW_H_

#include "sky/engine/core/events/Event.h"
#include "sky/engine/core/frame/DOMWindowBase64.h"
#include "sky/engine/core/frame/FrameDestructionObserver.h"
#include "sky/engine/platform/Supplementable.h"
#include "sky/engine/platform/heap/Handle.h"

#include "sky/engine/wtf/Forward.h"
#include "sky/engine/wtf/HashSet.h"

namespace blink {

class CSSStyleDeclaration;
class DOMURL;
class DOMWindowCSS;
class DOMWindowProperty;
class Database;
class DatabaseCallback;
class Document;
class DocumentInit;
class Element;
class EventListener;
class ExceptionState;
class FloatRect;
class IDBFactory;
class LocalFrame;
class MediaQueryList;
class Node;
class Page;
class RequestAnimationFrameCallback;
class ScheduledAction;
class ScriptCallStack;
class StyleMedia;
class Tracing;

enum PageshowEventPersistence {
    PageshowEventNotPersisted = 0,
    PageshowEventPersisted = 1
};

class LocalDOMWindow final : public DartWrappable, public RefCounted<LocalDOMWindow>, public DOMWindowBase64, public FrameDestructionObserver, public Supplementable<LocalDOMWindow> {
    DEFINE_WRAPPERTYPEINFO();
public:
    static PassRefPtr<LocalDOMWindow> create(LocalFrame& frame)
    {
        return adoptRef(new LocalDOMWindow(frame));
    }
    virtual ~LocalDOMWindow();

    PassRefPtr<Document> installNewDocument(const DocumentInit&);

    void AcceptDartGCVisitor(DartGCVisitor& visitor) const override;

    void registerProperty(DOMWindowProperty*);
    void unregisterProperty(DOMWindowProperty*);

    void reset();

    PassRefPtr<MediaQueryList> matchMedia(const String&);

    // DOM Level 0

    int outerHeight() const;
    int outerWidth() const;
    int innerHeight() const;
    int innerWidth() const;

    Tracing& tracing() const;

    // FIXME(sky): keeping self for now since js-test.html uses it.
    LocalDOMWindow* window() const;

    // DOM Level 2 AbstractView Interface

    Document* document() const;

    // CSSOM View Module

    StyleMedia& styleMedia() const;

    // DOM Level 2 Style Interface

    PassRefPtr<CSSStyleDeclaration> getComputedStyle(Element*) const;

    // WebKit extensions

    double devicePixelRatio() const;

    void printErrorMessage(const String&);

    void moveBy(float x, float y) const;
    void moveTo(float x, float y) const;

    void resizeBy(float x, float y) const;
    void resizeTo(float width, float height) const;

    // WebKit animation extensions
    int requestAnimationFrame(PassOwnPtr<RequestAnimationFrameCallback>);
    void cancelAnimationFrame(int id);

    DOMWindowCSS& css() const;

    // This is the interface orientation in degrees. Some examples are:
    //  0 is straight up; -90 is when the device is rotated 90 clockwise;
    //  90 is when rotated counter clockwise.
    int orientation() const;

    void willDetachDocumentFromFrame();

    bool isInsecureScriptAccess(LocalDOMWindow& callingWindow, const String& urlString);

private:
    explicit LocalDOMWindow(LocalFrame&);

    Page* page();

    virtual void frameDestroyed() override;
    virtual void willDetachFrameHost() override;

    void clearDocument();
    void resetDOMWindowProperties();
    void willDestroyDocumentInFrame();

    RefPtr<Document> m_document;

#if ENABLE(ASSERT)
    bool m_hasBeenReset;
#endif

    HashSet<DOMWindowProperty*> m_properties;

    mutable RefPtr<Tracing> m_tracing;
    mutable RefPtr<DOMWindowCSS> m_css;
};

} // namespace blink

#endif  // SKY_ENGINE_CORE_FRAME_LOCALDOMWINDOW_H_
