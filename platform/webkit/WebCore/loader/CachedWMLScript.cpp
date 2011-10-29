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
#include "CachedWMLScript.h"
#include "WMLDocument.h"
#include "WMLScriptInterface.h"

namespace WebCore {

WMLScriptLoader::WMLScriptLoader() 
    :m_wmlDoc(0)
    ,m_script(0)
{ 
}

void WMLScriptLoader::notifyFinished(CachedResource* resource)
{
    static_cast<CachedWMLScript*>(resource)->evaluateScript(m_wmlDoc) ;
    resource->removeClient(this) ;
}


CachedWMLScript::CachedWMLScript(const String& url, const String& charset)
    :CachedResource(url, WmlScript)
{

}

CachedWMLScript::~CachedWMLScript()
{
}


void CachedWMLScript::didAddClient(CachedResourceClient* c)
{
    if (!m_loading)
        c->notifyFinished(this);
}

void CachedWMLScript::allClientsRemoved()
{
    //m_decodedDataDeletionTimer.startOneShot(0);
}

void CachedWMLScript::data(PassRefPtr<SharedBuffer> data, bool allDataReceived)
{
    if(!allDataReceived )
        return ;

    m_data = data ;

    setEncodedSize(m_data.get() ? m_data->size() : 0);
    m_loading = false;
    
    checkNotify() ;
}

void CachedWMLScript::evaluateScript(WMLDocument *doc)
{
    String ScriptUrl = url() ;
    String scriptMimetype = m_response.mimeType() ;
    char* bufferval = NULL ; 
    unsigned int bufferLength = 0 ; 

    //m_buffer = ( char *)data->data();
    //m_bufferLen = data->size();
    WMLScript * wmlScript = doc->scriptInterface() ;

    if(m_data.get()) {
        bufferval = ( char *) m_data->data() ; 
        bufferLength = m_data->size() ; 
    }
    
    if ( scriptMimetype == "text/vnd.wap.wmlscript" )
    {
        wmlScript->bufferContent ( ScriptUrl, bufferval, bufferLength ) ;
        //bufferContent(ScriptUrl, bufferval, bufferLength ) ;  
    }
    else if( scriptMimetype == "text/vnd.wap.wmlsc" )
    {
        wmlScript->setCompiledBuffer(ScriptUrl, bufferval, bufferLength ) ;
        //setcompiledBuffer( ScriptUrl, bufferval, bufferLength ) ;
    }

    if (true == wmlScript->isWmlScriptSuspended())
    {
        if (wmlScript->downloadUrl() == ScriptUrl)
        {
            /* the content type of the load string should start with "text/" */
            if (scriptMimetype.startsWith(String("text/")))
            {
                /* The content can be displayed to the user eg. in case of load string */
                wmlScript->setBuffer (bufferval, bufferLength) ;
                //setInterfaceBuffer(bufferval, bufferLength) ;
            }
            else
            {
                /* The content is not a valid content as the content types do not match */
                wmlScript->setBuffer (NULL, 0) ;
                //setInterfaceBuffer(NULL, 0) ;
            }
            //CString ctype = scriptMimetype.utf8() ;
            wmlScript->setContentType(scriptMimetype) ;
            wmlScript->resumeScriptExec(NULL) ;
            // Reset the wmlScript->downloadUrl here
        }
        else if (ScriptUrl.contains(".wmls"))
        {
            wmlScript->resumeScriptExec(NULL) ;
        }

        
    }
    wmlScript->setWmlsecExecOnEventScript(ScriptUrl) ;
}

void CachedWMLScript::error()
{
    m_loading = false;
    m_errorOccurred = true;
    checkNotify();
}


void CachedWMLScript::checkNotify()
{
    if (m_loading)
        return;

    CachedResourceClientWalker w(m_clients);
    while (CachedResourceClient* c = w.next())
        c->notifyFinished(this);
}


} // namespace WebCore

#endif
