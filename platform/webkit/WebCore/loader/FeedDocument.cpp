/*
 * Copyright 2010, Samsung Electronics Inc, All rights reserved
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

#if ENABLE(SEC_FEATURE_RSS)

#include "FeedDocument.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "Element.h"
#include "SegmentedString.h"
#include "XMLTokenizer.h"


#define TYPE_XML        "text/xml"
#define TYPE_APPXML     "application/xml"
#define TYPE_ATOM       "application/atom+xml"
#define TYPE_RDF        "application/rdf+xml"
#define TYPE_RSS        "application/rss+xml"
#define TYPE_MAYBE_FEED "application/vnd.webkit.maybe.feed"

#define NS_RDF          "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RSS          "http://purl.org/rss/1.0/"

#define RSS200_XSL      "content://com.android.browser.rss/webkit_rss_2.0.xsl"
#define RSS100_XSL      "content://com.android.browser.rss/webkit_rss_1.0.xsl"
#define ATOM_XSL        "content://com.android.browser.rss/webkit_atom_1.0.xsl"

namespace WebCore {

bool FeedDocument::canAccess(const KURL& url)
{
    if( url.string() == RSS100_XSL
        || url.string() == RSS200_XSL
        || url.string() == ATOM_XSL)
        return true ;

    return false ;
}

class FeedTokenizer : public XMLTokenizer {
public:
    typedef enum
    {
        FEED_TYPE_NONE = 0,
        FEED_TYPE_RSS200,   //RSS 0.91/0.92/2.0
        FEED_TYPE_RSS100,   //RSS 1.0
        FEED_TYPE_ATOM,     //Atom 1.0
    }FeedType;

    FeedTokenizer(Document*, FrameView* _view = 0);
    virtual ~FeedTokenizer();

    virtual void write(const SegmentedString&, bool appendData);
    virtual void finish();
        
private:
    FeedType feedTypeFromContent(const String& dataString) ;
    bool replaceXMLDeclaration(String& dataString, FeedType type) ;
    const char* xmlStyleSheet(FeedType type) ;
    const char * mimeTypeFromFeedType(FeedType type);   

    Document* m_doc;
    bool m_receivedFirstChunk ;
};

FeedTokenizer::FeedTokenizer(Document* doc, FrameView* _view)
    : XMLTokenizer(doc, _view)
    , m_doc(doc)
    , m_receivedFirstChunk(false)
{    
}    

FeedTokenizer::~FeedTokenizer()
{
}

void FeedTokenizer::write(const SegmentedString& s, bool appendData)
{
    if(!m_receivedFirstChunk) {
        String dataString = s.toString();
        FeedType type = feedTypeFromContent(dataString) ;
        
        if(FEED_TYPE_NONE != type) {
            replaceXMLDeclaration(dataString, type) ;
            if(m_doc->frame() && m_doc->frame()->loader()) {
                String mime = mimeTypeFromFeedType(type) ;
                m_doc->frame()->loader()->setResponseMIMEType(mime) ;
            }
        }
        
        m_receivedFirstChunk = true ;
        XMLTokenizer::write(dataString, appendData) ;
    }
    else {
        XMLTokenizer::write(s, appendData) ;
    }
}

void FeedTokenizer::finish()
{
    m_receivedFirstChunk = false ;
    XMLTokenizer::finish() ;
}


bool isFeedMIMEType(const String& type)
{
    if(type == TYPE_XML || type == TYPE_APPXML || type == TYPE_ATOM || type == TYPE_RDF || type == TYPE_RSS)
        return true ;

    return false ;
}

/**
 * @return the first occurrence of a character within a string buffer,
 *         or NULL if not found
 */
static const UChar* FindChar(char c, const UChar *begin, const UChar *end)
{
    for (; begin < end; ++begin) {
        if (*begin == c)
            return begin;
    }
    return NULL;
}

/**
 *
 * Determine if a substring is the "documentElement" in the document.
 *
 * All of our sniffed substrings: <rss, <feed, <rdf:RDF must be the "document"
 * element within the XML DOM, i.e. the root container element. Otherwise,
 * it's possible that someone embedded one of these tags inside a document of
 * another type, e.g. a HTML document, and we don't want to show the preview
 * page if the document isn't actually a feed.
 * 
 * @param   start
 *          The beginning of the data being sniffed
 * @param   end
 *          The end of the data being sniffed, right before the substring that
 *          was found.
 * @returns true if the found substring is the documentElement, false 
 *          otherwise.
 */
static bool IsDocumentElement(const UChar *start, const UChar* end)
{
    // For every tag in the buffer, check to see if it's a PI, Doctype or 
    // comment, our desired substring or something invalid.
    while ( (start = FindChar('<', start, end)) ) {
        ++start;
        if (start >= end)
            return false;
        
        // Check to see if the character following the '<' is either '?' or '!'
        // (processing instruction or doctype or comment)... these are valid nodes
        // to have in the prologue. 
        if (*start != '?' && *start != '!')
            return false;
        
        // Now advance the iterator until the '>' (We do this because we don't want
        // to sniff indicator substrings that are embedded within other nodes, e.g.
        // comments: <!-- <rdf:RDF .. > -->
        start = FindChar('>', start, end);
        if (!start)
            return false;
        
        ++start;
    }
    return true;
}

/**
 * Determines whether or not a string exists as the root element in an XML data
 * string buffer.
 * @param   dataString
 *          The data being sniffed
 * @param   substring
 *          The substring being tested for existence and root-ness.
 * @returns PR_TRUE if the substring exists and is the documentElement, FALSE
 *          otherwise.
 */
static bool containsTopLevelSubstring(const String& dataString, const char *substring) 
{
    int offset = dataString.find(substring);
    if (offset == -1)
        return false;
    
    const UChar *begin = dataString.characters();
    
    // Only do the validation when we find the substring.
    return IsDocumentElement(begin, begin + offset);
}

FeedTokenizer::FeedType FeedTokenizer::feedTypeFromContent(const String& dataString)
{
    FeedType type = FEED_TYPE_NONE ;

    if( containsTopLevelSubstring(dataString, "<rss") ) {
        // RSS 0.91/0.92/2.0
        type = FEED_TYPE_RSS200 ;
    }
    else if( containsTopLevelSubstring(dataString, "<feed") ) {
        // Atom 1.0
        type = FEED_TYPE_ATOM ;
    }
    else if(containsTopLevelSubstring(dataString, "<rdf:RDF") &&
            dataString.find(NS_RDF) != -1 &&
            dataString.find(NS_RSS) != -1 ) {
        // RSS 1.0
        type = FEED_TYPE_RSS100 ;
    }

    return type ;
}

const char * FeedTokenizer::xmlStyleSheet(FeedType type)
{
    switch(type)
    {
    case FEED_TYPE_RSS200:
        return RSS200_XSL ;
        break ;
    case FEED_TYPE_RSS100 :
        return RSS100_XSL ;
        break ;
    case FEED_TYPE_ATOM :
        return ATOM_XSL ;
        break ;
    default:
        return RSS200_XSL ;
        break ;
    }
}

const char * FeedTokenizer::mimeTypeFromFeedType(FeedType type)
{
    switch(type)
    {
    case FEED_TYPE_RSS200:
        return TYPE_RSS ;
        break ;
    case FEED_TYPE_RSS100 :
        return TYPE_RDF ;
        break ;
    case FEED_TYPE_ATOM :
        return TYPE_ATOM;
        break ;
    }
    return TYPE_RSS ;
}

bool FeedTokenizer::replaceXMLDeclaration(String& dataString, FeedType type)
{
    int i, start, end ;
    bool replaced = false;
    String out = dataString.stripWhiteSpace() ;
    String xsl = xmlStyleSheet(type) ;
    String xslDeclaration = "<?xml-stylesheet href=\"" + xsl + "\" type=\"text/xsl\"?>" ;

    i = out.find("<?xml ") ;
    if(i != -1)
        i = out.find("?>", i + 6) ;
    if(i == -1)
        i = -2 ;

    start = out.find("<?xml-stylesheet", i+2) ;
    if(start != -1) {
        end = out.find("?>", start + 16) ;
        if(end != -1) {
            out.replace(start, ((end+2) - start), xslDeclaration) ;
            replaced = true ;
        }
    }
    else {
        out.insert(xslDeclaration, i+2) ;
        replaced = true ;
    }

    if(replaced)
        dataString = out ;

    return replaced;
}

FeedDocument::FeedDocument(Frame* frame)
    : Document(frame, false)
{
}

Tokenizer* FeedDocument::createTokenizer()
{
    return new FeedTokenizer(this, view());
}

}

#endif
