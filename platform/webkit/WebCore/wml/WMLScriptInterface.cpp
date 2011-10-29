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

#include "config.h"

#if ENABLE(WML) && ENABLE(WMLSCRIPT)

#include "Wmlsec.h"
#include "Wmls.h"

#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>
#include <wtf/PassRefPtr.h>

#include "PlatformString.h"
#include "StringHash.h"
#include "CString.h"
#include "KURL.h"
#include "Event.h"
#include "Page.h"
#include "NodeList.h"
#include "DocLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "ChromeClient.h"
#include "HTMLCollection.h"

#include "WMLCardElement.h"
#include "WMLDocument.h"
#include "WMLGoElement.h"
#include "WMLTimerElement.h"
#include "WMLScriptInterface.h"
#include "Cache.h"
#include "CachedWMLScript.h"


namespace WebCore {

static int RunWmlScriptAlert(void *closure, const char *message)
{
    static_cast<WMLScript*>(closure)->WmlScriptAlert(message);
    return  0;
}

static int RunWmlScriptConfirm(void *closure, const char *message, const char *okText, const char *cancelText)
{
    return static_cast<WMLScript*>(closure)->WmlScriptConfirm(message,okText,cancelText);
}

static int RunWmlScriptPrompt(void *closure, const char *message, const char *defInp)
{
    return static_cast<WMLScript*>(closure)->WmlScriptPrompt(message,defInp);
}

static void ExtUrlDownload (void *closure, char *Value)
{
    //performs the download of a given external url
    return static_cast<WMLScript*>(closure)->downloadExternalURL (Value) ;
}

static void DownloadUrl (void *closure, char *Value, char *contentType)
{
    //performs the download of a given url
    return static_cast<WMLScript*>(closure)->loadUrl (Value, contentType) ;
}

static void newBrowserContext (void *closure)
{
    //performs the same as newcontext attribute
    return static_cast<WMLScript*>(closure)->newBrowserContext () ;
}

static char* getCurrentCard (void *closure)
{
    //returns the current active card ID.
    return static_cast<WMLScript*>(closure)->getCurrentCard () ;
}

static char *getCurrentUrl (void *closure)
{
    //returns the current document url
    return static_cast<WMLScript*>(closure)->getCurrentUrl () ;
}

static char *getContextVariableValue (void *closure, char* name)
{
    //returns the value of the variable in the form of character string
    return static_cast<WMLScript*>(closure)->getContextVariableValue (name) ;
}
static BOOL isValidVariable(void *closure, char* name)
{
    //returns whether the variable is present in the browser context
    return static_cast<WMLScript*>(closure)->isValidVariable (name) ;
}

static void refreshBrowserDisplay (void *closure)
{
    //based on the current context, the browser display is updated
    return static_cast<WMLScript*>(closure)->refreshBrowserDisplay () ;
}

static void setVariableValue (void *closure, char* name, char * value)
{
    return static_cast<WMLScript*>(closure)->setVariableValue (name, value) ;
}

static void setGoElement(void *closure, char* Value )
{
    return static_cast<WMLScript*>(closure)->setGoElement (Value) ;
}
static void setPrevElement(void *closure, char *Value)
{
    return static_cast<WMLScript*>(closure)->setPrevElement (Value) ;
}

static void resumeWMLScriptExec(void *closure, char *Value)
{
    return static_cast<WMLScript*>(closure)->resumeScriptExec(Value) ;
}

static void InitializeCallbacks (WMLS_BRWFUNC_HANDLER *pHandle)
{
    pHandle->BrwCtx = newBrowserContext ;
    pHandle->BrwCurrCard = getCurrentCard;
    pHandle->BrwCurrUrl = getCurrentUrl ;
    pHandle->BrwgetVar = getContextVariableValue ;
    pHandle->BrwRefreshDoc = refreshBrowserDisplay ;
    pHandle->BrwsetVar = setVariableValue ;
    pHandle->BrwGoValue = setGoElement ;
    pHandle->BrwPrevValue = setPrevElement ;
    pHandle->BrwResScriptExecValue = resumeWMLScriptExec ;

    pHandle->BrwDownloadExtUrlValue = ExtUrlDownload ;
    pHandle->BrwDownloadUrlValue = DownloadUrl ;

    pHandle->BrwRunAlert = RunWmlScriptAlert;
    pHandle->BrwRunConfirm = RunWmlScriptConfirm;
    pHandle->BrwRunPrompt = RunWmlScriptPrompt;
    return ;
}

String WMLScript::getScriptURL(const String& url, Document* doc)
{
    String urlFragment ;
    String scriptUrl = url ;
    int pos = url.reverseFind('#') ;
    if(pos != -1) {
        urlFragment = url.substring(pos+1) ;
        scriptUrl = url.substring(0,pos);
    }
    String targetUrl = doc->completeURL(scriptUrl);
    if ( targetUrl.endsWith(".wmls") || IsScriptFragment(urlFragment))
        return targetUrl ;
    
    return String() ;
}

String WMLScript::getScriptFragment(const String& url)
{
    int pos = url.reverseFind('#') ;
    if(pos != -1)
        return url.substring(pos+1) ;
    
    return String() ;
}

bool WMLScript::IsScriptFragment(const String& fragment)
{
    int index = fragment.find('(') ;
    if(index != -1 && fragment.endsWith(")"))
        return true ;

    return false ;
}

WMLScript::WMLScript(WMLDocument *doc)
    :m_doc (doc)
{
    WMLSECINPUTPARAMS params ;
    WMLS_BRWFUNC_HANDLER callbacks ;
    ASSERT (m_doc) ;
    params.pWMLSIntfClass = this ;
    InitializeCallbacks (&callbacks) ;
    params.brwFunchandler = callbacks ;
    WmlsecInit(&params, &m_scriptHandle);
}

WMLScript::~WMLScript()
{
    WmlsecDeInit(m_scriptHandle);
    m_scriptHandle = NULL;

    //for(int i=0; i< m_cachedScripts.size(); i++)
    //  m_cachedScripts[i]->removeClient(this) ;

    //m_cachedScripts.clear() ;
}

void WMLScript::WmlScriptAlert(const char *message)
{
    m_doc->page()->chrome()->client()->runJavaScriptAlert(m_doc->frame(), message); 
}

bool WMLScript::WmlScriptConfirm(const char *message,const char *okText,const char *cancelText)
{
    return m_doc->page()->chrome()->client()->runJavaScriptConfirm(m_doc->frame(), message);        
}

bool WMLScript::WmlScriptPrompt(const char *message,const char *defInp)
{
    String result ;
    return m_doc->page()->chrome()->client()->runJavaScriptPrompt(m_doc->frame(), message, defInp, result);
}

void WMLScript::setGoElement ( char* Value)
{
    m_goURL = Value ; 
    return ;
}

void WMLScript::setPrevElement (char *value)
{
    prevElt = value ;
    return ;
}

void WMLScript::newBrowserContext ()
{
    //m_doc->frame()->wmlBrowserContext()->ResetBrowserContext() ;
    wmlPageStateForDocument(static_cast<Document*>(m_doc))->reset() ;
}

char* WMLScript::getCurrentCard ()
{
    const String& cardId = m_doc->activeCardId () ;
    CString utf8Str = cardId.utf8() ;
    char *cardIdStr = 0 ;

    if (!cardId.isEmpty())
        cardIdStr = (char*)fastMalloc(utf8Str.length()+1);
    
    if (cardIdStr)
        strcpy(cardIdStr, utf8Str.data()) ;

    return cardIdStr ;
}

char* WMLScript::getCurrentUrl ()
{
    CString utf8Str = m_doc->url().string().utf8() ;
    char *documentUrl = (char *)utf8Str.data() ;
    char *url = 0 ;
    
    if (documentUrl)
        url = (char*)fastMalloc(utf8Str.length()+1);
    
    if (url)
        strcpy(url, utf8Str.data()) ;
    
    return url ;
}

char *WMLScript::getContextVariableValue (char *name)
{
    String tempName(name) ;
    //String value = m_doc->frame()->wmlBrowserContext()->GetVariableValue(tempName) ;
    String value = wmlPageStateForDocument(static_cast<Document*>(m_doc))->getVariable(tempName) ;
    CString utf8Str = value.utf8() ;
    char *tempValue = (char *)(utf8Str.data()) ;
    char *pValue = 0 ;
    
    if (tempValue)
        pValue = (char*)fastMalloc(utf8Str.length()+1);
    
    if (pValue)
        strcpy(pValue, tempValue) ;
    
    return pValue ;
}

bool WMLScript::isValidVariable (char *name)
{
    //not required to be implemented
    return TRUE ;
}

void WMLScript::refreshBrowserDisplay ()
{   
    RefPtr<NodeList> nodeList = m_doc->getElementsByTagName("timer");
    if(nodeList){
        unsigned length = nodeList->length();

        for(unsigned i = 0; i<length; ++i) {
            WMLTimerElement *timer = static_cast<WMLTimerElement*>(nodeList->item(i)) ;
            timer->storeIntervalToPageState();
            timer->stop();
        }
    }

    if (Frame* frame = m_doc->frame()) {
        if (FrameLoader* loader = frame->loader())
            loader->reload();
    }

}

void WMLScript::setVariableValue (char *name, char *value)
{
    String tempName(name) ;
    String tempValue(value) ;
    //wmlPageStateForDocument(m_doc)->storeVariable(tempName, tempValue) ;
    wmlPageStateForDocument(static_cast<Document*>(m_doc))->storeVariable(tempName, tempValue) ;
}

// It's for raw buffer
void WMLScript::bufferContent ( const String& url, char *buffer, int bufferLen )
{
    String charset = m_doc->charset() ;
    CompileContent( url, buffer, bufferLen,  charset ) ; 
}

void WMLScript::CompileContent(  const String& url, char* buffer, int bufferLen, String charset )
{
    WMLSRETCODE rCode ;
    int charsetVal = 0x6A ;

    rCode = WmlsecCompileScript(m_scriptHandle, buffer, bufferLen, charsetVal) ;
    if (RCODE_OK)
        setCompiledBuffer ( url, ( char * )WMLS_GET_COMPILEDBUFF(m_scriptHandle), WMLS_GET_COMPILEDBUFFLEN(m_scriptHandle) ) ; 

    return ;
}

/*Process the WMLBrowser.go() or WMLBrowser.prev() navigation actions*/
void WMLScript::processScriptNavigation() 
{
    if (!m_goURL.isEmpty()) {
        ResourceRequest request;
        KURL targetGoUrl = m_doc->completeURL (m_goURL ) ;

        request.setURL(targetGoUrl) ; 
        m_doc->frame()->loader()->urlSelected( request, String(), NULL, false, false, true) ;
    }
    else if(prevElt) {
        m_doc->page()->goBack() ;
    }
}

void WMLScript::resumeScriptExec(char *fragment) 
{
    WMLSRETCODE rCode ;
    WMLSINPUTPARAMS WmlsInputParams  ;

    memset(&WmlsInputParams, 0x00, sizeof(WMLSINPUTPARAMS) );
    WmlsInputParams.InpBuff = m_compiledBuffer ;
    WmlsInputParams.InpBuffLen = m_compiledBufferLen ;
    WmlsInputParams.pFragment = fragment ;

    CString latinStr = m_contentType.latin1() ;
    if (!m_contentType.isEmpty()) {
        WmlsInputParams.urlContenttype =(char*)latinStr.data() ;
    }

    rCode = WmlsecResumeExecution(m_scriptHandle, &WmlsInputParams) ;
    if (RCODE_OK)
        processScriptNavigation() ;

    return ;
}

void WMLScript::exeScript(const String& targetUrlPrefix, String &fragment ) 
{
    WMLSRETCODE rCode ;
    String DocumentUrl = m_doc->baseURL();
    CString frag = fragment.utf8() ;
    CString docURL = DocumentUrl.utf8() ;
    WMLSINPUTPARAMS WmlsInputParams  ;

    memset(&WmlsInputParams, 0x00, sizeof(WMLSINPUTPARAMS)) ;
    getCompiledBuffer ( targetUrlPrefix, &m_compiledBuffer, &m_compiledBufferLen) ;
    
    /*  Note:m_compiledBufferLen = 0 indicates script syntax error or an unfetched url error. 
    WML will continue with processing from the next element */
    if (m_compiledBufferLen) {
        WmlsInputParams.InpBuff = m_compiledBuffer ;
        WmlsInputParams.InpBuffLen = m_compiledBufferLen ;

        WmlsInputParams.pFragment = frag.data() ;
        WmlsInputParams.pDocUrl =  docURL.data() ;

        m_goURL = String() ;
        prevElt = 0 ;

        rCode = WmlsecExecuteScript(m_scriptHandle, &WmlsInputParams) ;     
        if (RCODE_OK)
            processScriptNavigation() ;

        delete m_compiledBuffer ;
        m_compiledBuffer = 0 ;
        m_compiledBufferLen = 0 ;
    }
    return ;
}

void WMLScript::setCompiledBuffer( const String& url, char * buffer, int bufferLen )
{
    WMLCompiledScript tempWMLCompiledScript = {bufferLen, buffer};
    m_wmlCompiledScriptMap.add(url, tempWMLCompiledScript); 
    //addCompiledScript(url,buffer, bufferLen); 
}

void WMLScript::getCompiledBuffer ( const String& url, char **app_buffer, int *p_buflen )
{
    CachedWMLScript *var = static_cast<CachedWMLScript*>(cache()->resourceForURL(url)) ;
    //CachedWMLScript *var = m_cachedScriptMap.get( url ) ;
    if ( var ) {
        int iIndex ;
        WMLCompiledScript compiledScript = m_wmlCompiledScriptMap.get(url);
        int bufferLen = compiledScript.bufferlen;
        *app_buffer = new char[bufferLen];
        for (iIndex=0; iIndex<bufferLen; iIndex++) {    
            (*app_buffer)[iIndex] = compiledScript.bufferContent[iIndex] ;
        }
        *p_buflen= bufferLen ;
    }
}

//void WMLScript::addCachedScript (const String &url, CachedWMLScript* cachedScript)    
//{
//  if (m_cachedScriptMap.contains(url))
//      return ;
//  m_cachedScriptMap.add(url, cachedScript ) ;
//}

//void WMLScript::addCompiledScript(const String &url, char *buffer, int len)
//{
//  WMLCompiledScript tempWMLCompiledScript = {len, buffer};
//  m_wmlCompiledScriptMap.add(url, tempWMLCompiledScript);
//}

void WMLScript::setBuffer (char * buffer, int bufferLen )   
{
    m_compiledBuffer = buffer ;
    m_compiledBufferLen = bufferLen ;
}

void WMLScript::requestScript(const String& url, const String& charset)
{
    CachedWMLScript *script ;
    WMLScriptLoader *loader = static_cast<WMLScriptLoader*>(this) ;
    
    loader->setDocument(m_doc) ;
    script = m_doc->docLoader()->requestWMLScript( url, charset ) ;
    script->addClient(static_cast<CachedResourceClient*>(loader)) ;
    
    m_cachedScripts.append(script) ;
}

void WMLScript::downloadExternalURL (char *UrltoDownload)
{
    String URLValue = String ( UrltoDownload );
    String targetGoUrl = m_doc->completeURL (URLValue ) ;
/*  CachedWMLScript *var = m_cachedScriptMap.get( targetGoUrl ) ;

    if ( var ) {
        char *scriptBuffer = NULL;
        int scriptBufferLen = 0;
        var->getBuffer (&scriptBuffer, &scriptBufferLen ) ;
        WMLS_SET_COMPILEDBUFF(m_scriptHandle,(unsigned char *)scriptBuffer);
        WMLS_SET_COMPILEDBUFFLEN(m_scriptHandle,scriptBufferLen);
    }
    else*/ {
        requestScript( targetGoUrl, String()) ;
    }
    return ;
}

void WMLScript::loadUrl (char *UrltoDownload, char *contentype)
{ 
    String URLValue = String ( UrltoDownload );
    ASSERT(NULL!=contentype) ;
    WMLSEC_SET_CONTENT_TYPE(m_scriptHandle, contentype) ;
    m_downloadUrl = URLValue ;

    requestScript( URLValue, String()) ;

    return ;
}

bool WMLScript::isWmlScriptSuspended()
{
    if (NULL == m_scriptHandle) {
        //Ideally this (m_scriptHandle to be NULL)should never happen
        //But occassinally when the document is destroyed it happens
        //This seems to work most of the time. But still root cause needs to be identified
        //We need to find where this m_scriptHandle pointer is allocated/deallocated and resolve it
        return false;
    }

    return IS_WMLSE_SUSPEND(m_scriptHandle) ;
}

bool WMLScript::isWmlScriptExecuting()
{
    return IS_WMLSE_EXECUTING(m_scriptHandle) ;
}

bool WMLScript::isWmlScriptIdle()
{
    return IS_WMLSE_IDLE(m_scriptHandle) ;
}

bool WMLScript::isWmlScriptOnEventExecuting()
{
    if (NULL == m_scriptHandle)
        return false;

    return IS_WMLSE_ONEVENTEXECUTING(m_scriptHandle) ;
}

void WMLScript::setWmlsecStateOnEventExecuting()
{
    SET_WMLSE_STATE_ONEVENT_ENCOUNTERED(m_scriptHandle);
    return;
}

void WMLScript::setWmlsecOnEventParams(String &url, String &fragment)
{
    /* memory will be allocated by the wmlsec controller 
       no need to allocate fresh memory for url and fragment by the caller */
    m_onEventUrl = url ;
    m_onEventFragment = fragment ;
    /*Shall assert if the script enginr is in suspended/executing state */
    setWmlsecStateOnEventExecuting();
}

void WMLScript::setWmlsecExecOnEventScript(String &scriptUrl)
{
    /* 
    1. perform url compare
    2. if 1 is success, perform exeScript
    3. Otherwise, just return
    */
    if (isWmlScriptOnEventExecuting() && (scriptUrl == m_onEventUrl))
        exeScript(m_onEventUrl, m_onEventFragment) ;

    return ;

}

}//WebCore

#endif  //ENABLE(WML) && ENABLE(WMLSCRIPT)
