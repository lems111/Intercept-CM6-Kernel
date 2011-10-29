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

#if ENABLE(WML)

#include "StringImpl.h"
#include "WMLTokenizer.h"

namespace WebCore {


WMLTokenizer::WMLTokenizer(Document* doc, FrameView* view)
    : XMLTokenizer(doc, view)
    , m_receivedFirstChunk(false)
{    
}    

WMLTokenizer::~WMLTokenizer()
{
}

void WMLTokenizer::write(const SegmentedString& s, bool appendData)
{
    if(!m_receivedFirstChunk) {
        SegmentedString src(s);
        
        // skip white space from start
        while (src.length()>1 && isSpaceOrNewline(*src))
            src.advance() ;

        m_receivedFirstChunk = true ;
        
        XMLTokenizer::write(src, appendData) ;
    }
    else
        XMLTokenizer::write(s, appendData) ;
}

void WMLTokenizer::finish()
{
    m_receivedFirstChunk = false ;
    XMLTokenizer::finish() ;
}

}

#endif

