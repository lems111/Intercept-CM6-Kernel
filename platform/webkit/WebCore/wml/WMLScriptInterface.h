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

#ifndef WMLScriptInterface_h
#define WMLScriptInterface_h

#include "config.h"

#if ENABLE(WML) && ENABLE(WMLSCRIPT)
#include <wtf/HashMap.h>

#include "PlatformString.h"
#include "StringHash.h"
#include "FrameLoader.h"
#include "CachedWMLScript.h"

using namespace WebCore ;

namespace WebCore {

class WMLDocument;
class CachedWMLScript;                      

typedef struct WMLComScript
{
    int bufferlen;
    char *bufferContent;
}WMLCompiledScript;

typedef HashMap<String, CachedWMLScript *> WMLScriptMap;
typedef HashMap<String, WMLCompiledScript> WMLcompiledScriptMap;

class WMLScript : public WMLScriptLoader
{
public:

    static String getScriptURL(const String& url, Document* doc) ;
    static String getScriptFragment(const String& url) ;
    static bool IsScriptFragment(const String& fragment) ;

    WMLScript (WMLDocument *doc) ;
    ~WMLScript() ;

    void CompileContent( const String& url,char* buffer, int bufferLen, String charset ) ;

    void bufferContent (const String& url, char *buffer, int bufferLen ) ;
    void exeScript(const String& targetUrlPrefix, String &fragment ) ;
    
    void setCompiledBuffer( const String& url, char *buffer, int bufferLen ) ;
    void getCompiledBuffer( const String& url, char **app_buffer, int *p_buflen) ; 
    
    //void addCachedScript (const String &url, CachedWMLScript* cachedScript) ;
    //void addCompiledScript(const String &url, char* compiledScript, int len);

    void setBuffer (char * buffer, int bufferLen ) ;

    //callback interfaces for the browser context
    void newBrowserContext () ;
    char *getCurrentCard () ;
    char *getCurrentUrl () ;
    char *getContextVariableValue (char *name) ;
    bool isValidVariable (char *name) ;
    void refreshBrowserDisplay () ;
    void setVariableValue (char *name, char *value) ;
    void setGoElement ( char* value) ;
    void setPrevElement (char *value) ;
    void resumeScriptExec(char *value) ;

    void downloadExternalURL(char *value) ;
    void loadUrl(char *value, char *contentype) ;

    bool isWmlScriptSuspended() ;
    bool isWmlScriptExecuting() ;
    bool isWmlScriptIdle() ;
    bool isWmlScriptOnEventExecuting() ;

    void setWmlsecStateOnEventExecuting() ;

    void setWmlsecOnEventParams(String &url, String &fragment) ;
    void setWmlsecExecOnEventScript(String &scriptUrl) ;

    void WmlScriptAlert(const char *message);
    bool WmlScriptConfirm(const char *message,const char *okText,const char *cancelText);
    bool WmlScriptPrompt(const char *message,const char *defInp);
    
    WMLScriptMap getWMLSCachedScript(){return m_cachedScriptMap;}

    const String& downloadUrl()const { return m_downloadUrl; }
    void setContentType(const String& type) { m_contentType = type; }

private:
    void processScriptNavigation() ;
    void requestScript(const String& url, const String& charset) ;

private:

    Vector<CachedWMLScript *> m_cachedScripts ;
    WMLScriptMap m_cachedScriptMap ;

    WMLcompiledScriptMap m_wmlCompiledScriptMap;
    
    WMLDocument* m_doc ;

    char* prevElt ;
    char* m_compiledBuffer ;

    int m_compiledBufferLen ;

    // Needed as it will be accessed by the cached script also
    void* m_scriptHandle ;
    
    String m_goURL ;
    String m_contentType ;
    String m_downloadUrl ;
    String m_onEventUrl ;
    String m_onEventFragment ;

};

}

#endif //ENABLE(WML) && ENABLE(WMLSCRIPT)

#endif  //WMLScriptInterface_h
