/*
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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

#ifndef WMLDocument_h
#define WMLDocument_h

#if ENABLE(WML)
#include "Document.h"
#include "WMLErrorHandling.h"
#include "WMLPageState.h"
#include "Timer.h"

namespace WebCore {

class WMLCardElement;

#if ENABLE(WMLSCRIPT)
class WMLScript ; 
#endif

class WMLDocument : public Document {
public:
    static PassRefPtr<WMLDocument> create(Frame* frame)
    {
        return new WMLDocument(frame);
    }

    virtual ~WMLDocument();

    virtual bool isWMLDocument() const { return true; }
    virtual void finishedParsing();
    virtual Tokenizer* createTokenizer();

    bool initialize(bool aboutToFinishParsing = false);

    // SAMSUNG_WML_FIXES+
    void intrinsicEventTimerFired(Timer<WMLDocument>*);
    void setActiveCard(WMLCardElement *card) { m_activeCard = card; }
    // SAMSUNG_WML_FIXES-
    WMLCardElement* activeCard() const { return m_activeCard; }

#if ENABLE(WMLSCRIPT)
    WMLScript* scriptInterface (){return m_wmlScript;}
    void setActiveCardId(const String& cardId) { m_activeCardId = cardId; } 
    const String& activeCardId() { return m_activeCardId; }
#endif
private:
    WMLDocument(Frame*);
    WMLCardElement* m_activeCard;
#if ENABLE(WMLSCRIPT)
    WMLScript* m_wmlScript ; 
    String m_activeCardId;
#endif
    // SAMSUNG_WML_FIXES+
    Timer<WMLDocument> m_intrinsicEventTimer;
    // SAMSUNG_WML_FIXES-

};

WMLPageState* wmlPageStateForDocument(Document*);

}

#endif
#endif
