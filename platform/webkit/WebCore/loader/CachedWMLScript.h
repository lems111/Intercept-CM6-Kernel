/*
 * Copyright 2009, Samsung Electronics Inc, All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
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
  
#ifndef CachedWMLScript_h
#define CachedWMLScript_h

#include "config.h"

#if ENABLE(WML) && ENABLE(WMLSCRIPT)
#include "CachedResource.h"
#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"

namespace WebCore {

class WMLDocument;
class DocLoader;
class TextResourceDecoder;
class WMLScriptLoader ;
class CachedWMLScript : public CachedResource
{
public:

    CachedWMLScript(const String& URL, const String& charset) ;
    virtual ~CachedWMLScript();

    void createCachedResourceClient() ;

    virtual void didAddClient(CachedResourceClient*);
    virtual void allClientsRemoved();

    virtual void data(PassRefPtr<SharedBuffer> data, bool allDataReceived);
    virtual void error();
    void checkNotify() ;

    //void setDocument(WMLDocument *doc) {m_doc= doc;}

    void evaluateScript(WMLDocument *doc) ;
private:
    //WMLDocument *m_doc ;
    //RefPtr<TextResourceDecoder> m_decoder;
    WMLScriptLoader* m_loader;

};


class WMLScriptLoader : public CachedResourceClient {
public:
    WMLScriptLoader();

    void setDocument(WMLDocument *doc) { m_wmlDoc = doc; }
    virtual void notifyFinished(CachedResource* resource);

protected:
    WMLDocument *m_wmlDoc ;
    CachedWMLScript* m_script ;
};

}

#endif

#endif
