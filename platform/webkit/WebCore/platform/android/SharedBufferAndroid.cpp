/*
 * Copyright (C) 2009 Samsung Electronics Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
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

#include "config.h"

#include "CString.h"
#include "FileSystem.h"
#include "SharedBuffer.h"

#include <fnmatch.h>
#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

namespace WebCore {

#if defined(SAMSUNG_ANDROID_FILE_UPLOAD)
PassRefPtr<SharedBuffer> SharedBuffer::createWithContentsOfFile(const String& filePath)
{
    RefPtr<SharedBuffer> result;
    int fileDescriptor = -1;
    long long totalBytesRead = 0;
    long long fileSize = 0;
    CString filename = filePath.utf8();

    if (filePath.isEmpty())
        goto exit;

    if( !getFileSize(filePath, fileSize) ) 
        goto exit;

    fileDescriptor = open(filename.data(), O_RDONLY , S_IRUSR);
    if (fileDescriptor == -1)
        goto exit;


    result = SharedBuffer::create();
    result->m_buffer.resize(fileSize);
    if (result->m_buffer.size() != fileSize) {
        result = 0;
        goto exit;
    }

    totalBytesRead = read(fileDescriptor, (void*) result->m_buffer.data(), (size_t)fileSize);
    if (totalBytesRead != fileSize) {
        result = 0;
        goto exit;
    }

exit:
    close(fileDescriptor);
    return result.release();
}  
#endif

}; // namespace WebCore
