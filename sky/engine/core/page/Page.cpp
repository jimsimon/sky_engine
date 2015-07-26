/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All Rights Reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

#include "sky/engine/core/page/Page.h"

#include "sky/engine/core/dom/ClientRectList.h"
#include "sky/engine/core/dom/Document.h"
#include "sky/engine/core/dom/DocumentMarkerController.h"
#include "sky/engine/core/dom/StyleEngine.h"
#include "sky/engine/core/events/Event.h"
#include "sky/engine/core/frame/FrameHost.h"
#include "sky/engine/core/frame/FrameView.h"
#include "sky/engine/core/frame/LocalDOMWindow.h"
#include "sky/engine/core/frame/LocalFrame.h"
#include "sky/engine/core/frame/Settings.h"
#include "sky/engine/core/page/ChromeClient.h"
#include "sky/engine/core/rendering/RenderView.h"
#include "sky/engine/platform/geometry/FloatRect.h"
#include "sky/engine/wtf/HashMap.h"
#include "sky/engine/wtf/PassRefPtr.h"
#include "sky/engine/wtf/RefCountedLeakCounter.h"
#include "sky/engine/wtf/StdLibExtras.h"
#include "sky/engine/wtf/Vector.h"
#include "sky/engine/wtf/text/Base64.h"

namespace blink {

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, pageCounter, ("Page"));

float deviceScaleFactor(LocalFrame* frame)
{
    if (!frame)
        return 1;
    Page* page = frame->page();
    if (!page)
        return 1;
    return page->deviceScaleFactor();
}

Page::Page(PageClients& pageClients, ServiceProvider* services)
    : SettingsDelegate(Settings::create())
    , m_chromeClient(pageClients.chromeClient)
    , m_mainFrame(0)
    , m_deviceScaleFactor(1)
#if ENABLE(ASSERT)
    , m_isPainting(false)
#endif
    , m_frameHost(FrameHost::create(*this, services))
{
#ifndef NDEBUG
    pageCounter.increment();
#endif
}

Page::~Page()
{
    // willBeDestroyed() must be called before Page destruction.
    ASSERT(!m_mainFrame);
}

void Page::setMainFrame(LocalFrame* mainFrame)
{
    // Should only be called during initialization or swaps between local and
    // remote frames.
    // FIXME: Unfortunately we can't assert on this at the moment, because this
    // is called in the base constructor for both LocalFrame and RemoteFrame,
    // when the vtables for the derived classes have not yet been setup.
    m_mainFrame = mainFrame;
}

void Page::documentDetached(Document* document)
{
    m_multisamplingChangedObservers.clear();
}

void Page::setNeedsRecalcStyleInAllFrames()
{
    LocalFrame* frame = mainFrame();
    if (frame && frame->document())
        frame->document()->styleResolverChanged();
}

void Page::setNeedsLayoutInAllFrames()
{
    LocalFrame* frame = mainFrame();
    if (FrameView* view = frame->view()) {
        view->setNeedsLayout();
        view->scheduleRelayout();
    }
}

void Page::unmarkAllTextMatches()
{
    if (!mainFrame())
        return;

    mainFrame()->document()->markers().removeMarkers(DocumentMarker::TextMatch);
}

void Page::setDeviceScaleFactor(float scaleFactor)
{
    if (m_deviceScaleFactor == scaleFactor)
        return;

    m_deviceScaleFactor = scaleFactor;
    setNeedsRecalcStyleInAllFrames();

    if (mainFrame())
        mainFrame()->deviceOrPageScaleFactorChanged();
}

void Page::addMultisamplingChangedObserver(MultisamplingChangedObserver* observer)
{
    m_multisamplingChangedObservers.add(observer);
}

void Page::removeMultisamplingChangedObserver(MultisamplingChangedObserver* observer)
{
    m_multisamplingChangedObservers.remove(observer);
}

void Page::settingsChanged(SettingsDelegate::ChangeType changeType)
{
    switch (changeType) {
    case SettingsDelegate::StyleChange:
        setNeedsRecalcStyleInAllFrames();
        break;
    case SettingsDelegate::MediaTypeChange:
        if (mainFrame()) {
            mainFrame()->view()->setMediaType(AtomicString(settings().mediaTypeOverride()));
            setNeedsRecalcStyleInAllFrames();
        }
        break;
    case SettingsDelegate::MultisamplingChange: {
        HashSet<RawPtr<MultisamplingChangedObserver> >::iterator stop = m_multisamplingChangedObservers.end();
        for (HashSet<RawPtr<MultisamplingChangedObserver> >::iterator it = m_multisamplingChangedObservers.begin(); it != stop; ++it)
            (*it)->multisamplingChanged(m_settings->openGLMultisamplingEnabled());
        break;
    }
    case SettingsDelegate::ImageLoadingChange:
        break;
    case SettingsDelegate::FontFamilyChange:
        if (mainFrame()->document() && mainFrame()->document()->isActive())
            mainFrame()->document()->styleEngine()->updateGenericFontFamilySettings();
        setNeedsRecalcStyleInAllFrames();
        break;
    case SettingsDelegate::MediaQueryChange:
        if (mainFrame()->document())
            mainFrame()->document()->mediaQueryAffectingValueChanged();
        setNeedsRecalcStyleInAllFrames();
        break;
    }
}

void Page::didCommitLoad(LocalFrame* frame)
{
    ASSERT_NOT_REACHED();
}

void Page::willBeDestroyed()
{
    RefPtr<LocalFrame> mainFrame = m_mainFrame;

    mainFrame->detach();
    mainFrame->setView(nullptr);

#ifndef NDEBUG
    pageCounter.decrement();
#endif

    m_mainFrame = 0;
}

IntRect Page::rootViewToScreen(const IntRect& rect) const
{
    return m_chromeClient->rootViewToScreen(rect);
}

void Page::setWindowRect(const FloatRect& rect) const
{
    m_chromeClient->setWindowRect(rect);
}

FloatRect Page::windowRect() const
{
    return m_chromeClient->windowRect();
}

void Page::scheduleVisualUpdate()
{
    m_chromeClient->scheduleVisualUpdate();
}

bool Page::shouldReportDetailedMessageForSource(const String& source)
{
    return m_chromeClient->shouldReportDetailedMessageForSource(source);
}

void Page::addMessageToConsole(LocalFrame* localFrame, MessageSource source, MessageLevel level, const String& message, unsigned lineNumber, const String& sourceID, const String& stackTrace)
{
    m_chromeClient->addMessageToConsole(localFrame, source, level, message, lineNumber, sourceID, stackTrace);
}

void* Page::webView() const
{
    return m_chromeClient->webView();
}

Page::PageClients::PageClients()
    : chromeClient(0)
{
}

Page::PageClients::~PageClients()
{
}

} // namespace blink
