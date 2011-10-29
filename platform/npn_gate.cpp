/* Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is 
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or 
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.

*/
////////////////////////////////////////////////////////////
//
// Implementation of Netscape entry points (NPN_*)
// 
//

#include <stdlib.h>
#include "npapi.h"
#include "npfunctions.h"


static NPNetscapeFuncs NPNFuncs;

NPError NPN_GetURL(NPP instance, const char *url, const char *target)
{
    NPError rv = NPNFuncs.geturl(instance, url, target);
    return rv;
}



NPError NPN_PostURL(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file)
{
    NPError rv = NPNFuncs.posturl(instance, url, window, len, buf, file);
    return rv;
}


 
NPError NPN_RequestRead(NPStream* stream, NPByteRange* rangeList)
{
    NPError rv = NPNFuncs.requestread(stream, rangeList);
    return rv;
}



NPError NPN_NewStream(NPP instance, NPMIMEType type, const char* target, NPStream** stream)
{
    NPError rv = NPNFuncs.newstream(instance, type, target, stream);
    return rv;
}



int32_t NPN_Write(NPP instance, NPStream *stream, int32_t len, void *buffer)
{
    int32_t rv = NPNFuncs.write(instance, stream, len, buffer);
    return rv;
}



NPError NPN_DestroyStream(NPP instance, NPStream* stream, NPError reason)
{
    NPError rv = NPNFuncs.destroystream(instance, stream, reason);
    return rv;
}



void NPN_Status(NPP instance, const char *message)
{
    NPNFuncs.status(instance, message);
}



const char* NPN_UserAgent(NPP instance)
{
    const char * rv = NULL;
    rv = NPNFuncs.uagent(instance);
    return rv;
}



void* NPN_MemAlloc(uint32_t size)
{
    void * rv = NULL;
    rv = NPNFuncs.memalloc(size);
    return rv;
}



void NPN_MemFree(void* ptr)
{
    NPNFuncs.memfree(ptr);
}



uint32_t NPN_MemFlush(uint32_t size)
{
    uint32_t rv = NPNFuncs.memflush(size);
    return rv;
}



void NPN_ReloadPlugins(NPBool reloadPages)
{
    NPNFuncs.reloadplugins(reloadPages);
}



NPError NPN_GetURLNotify(NPP instance, const char *url, const char *target, void* notifyData)
{
    NPError rv = NPNFuncs.geturlnotify(instance, url, target, notifyData);
    return rv;
}



NPError NPN_PostURLNotify(NPP instance, const char* url, const char* window, uint32_t len, const char* buf, NPBool file, void* notifyData)
{
    NPError rv = NPNFuncs.posturlnotify(instance, url, window, len, buf, file, notifyData);
    return rv;
}



NPError NPN_GetValue(NPP instance, NPNVariable variable, void *value)
{
    NPError rv = NPNFuncs.getvalue(instance, variable, value);
    return rv;
}



NPError NPN_SetValue(NPP instance, NPPVariable variable, void *value)
{
    NPError rv = NPNFuncs.setvalue(instance, variable, value);
    return rv;
}



void NPN_InvalidateRect(NPP instance, NPRect *invalidRect)
{
    NPNFuncs.invalidaterect(instance, invalidRect);
}



void NPN_InvalidateRegion(NPP instance, NPRegion invalidRegion)
{
    NPNFuncs.invalidateregion(instance, invalidRegion);
}



void NPN_ForceRedraw(NPP instance)
{
    NPNFuncs.forceredraw(instance);
}



uint32 NPN_ScheduleTimer(NPP instance, uint32 interval, NPBool repeat, void (*timerFunc)(NPP instance, uint32 timerID))
{
    uint32 rv = NPNFuncs.scheduletimer(instance, interval, repeat, timerFunc);
    return rv;
}


void NPN_UnscheduleTimer(NPP instance, uint32 timerID)
{
    NPNFuncs.unscheduletimer(instance, timerID);
}





/*
    NPN_ReleaseVariantValue is called on all 'out' parameters references.
    Specifically it is called on variants that are resultant out parameters
    in NPGetPropertyFunctionPtr and NPInvokeFunctionPtr.  Resultant variants
    from these two functions should be initialized using the
    NPN_InitializeVariantXXX() functions.
    
    After calling NPReleaseVariantValue, the type of the variant will
    be set to NPVariantUndefinedType.
*/
void NPN_ReleaseVariantValue(NPVariant *variant)
{
    NPNFuncs.releasevariantvalue(variant);
}


/*
    NPObjects have methods and properties.  Methods and properties are
    identified with NPIdentifiers.  These identifiers may be reflected
    in script.  NPIdentifiers can be either strings or integers, IOW,
    methods and properties can be identified by either strings or
    integers (i.e. foo["bar"] vs foo[1]). NPIdentifiers can be
    compared using ==.  In case of any errors, the requested
    NPIdentifier(s) will be NULL.
*/
NPIdentifier NPN_GetStringIdentifier(const NPUTF8 *name)
{
    return NPNFuncs.getstringidentifier(name);
}



void NPN_GetStringIdentifiers(const NPUTF8 **names, int32_t nameCount, NPIdentifier *identifiers)
{
    return NPNFuncs.getstringidentifiers(names, nameCount, identifiers);
}



NPIdentifier NPN_GetIntIdentifier(int32_t intid)
{
    return NPNFuncs.getintidentifier(intid);
}



bool NPN_IdentifierIsString(NPIdentifier identifier)
{
    return NPNFuncs.identifierisstring(identifier);
}



/*
    The NPUTF8 returned from NPN_UTF8FromIdentifier SHOULD be freed.
*/
NPUTF8* NPN_UTF8FromIdentifier(NPIdentifier identifier)
{
    return NPNFuncs.utf8fromidentifier(identifier);
}



/*
    Get the integer represented by identifier. If identifier is not an
    integer identifier, the behaviour is undefined.
*/
int32_t NPN_IntFromIdentifier(NPIdentifier identifier)
{
    return NPNFuncs.intfromidentifier(identifier);
}





/*
    NPObjects returned by create have a reference count of one.  It is the caller's responsibility
    to release the returned object.

    NPInvokeFunctionPtr function may return false to indicate a the method could not be invoked.
    
    NPGetPropertyFunctionPtr and NPSetPropertyFunctionPtr may return false to indicate a property doesn't
    exist.
    
    NPInvalidateFunctionPtr is called by the scripting environment when the native code is
    shutdown.  Any attempt to message a NPObject instance after the invalidate
    callback has been called will result in undefined behavior, even if the
    native code is still retaining those NPObject instances.
    (The runtime will typically return immediately, with 0 or NULL, from an attempt to
    dispatch to a NPObject, but this behavior should not be depended upon.)
    
    The NPEnumerationFunctionPtr function may pass an array of                  
    NPIdentifiers back to the caller. The callee allocs the memory of           
    the array using NPN_MemAlloc(), and it's the caller's responsibility        
    to release it using NPN_MemFree().           
*/



/*
    If the class has an allocate function, NPN_CreateObject invokes that function,
    otherwise a NPObject is allocated and returned.  If a class has an allocate
    function it is the responsibility of that implementation to set the initial retain
    count to 1.
*/
NPObject *NPN_CreateObject(NPP instance, NPClass *aClass)
{
    return NPNFuncs.createobject(instance, aClass);
}



/*
    Increment the NPObject's reference count.
*/
NPObject* NPN_RetainObject(NPObject *obj)
{
    return NPNFuncs.retainobject(obj);
}



/*
    Decremented the NPObject's reference count.  If the reference
    count goes to zero, the class's destroy function is invoke if
    specified, otherwise the object is freed directly.
*/
void NPN_ReleaseObject(NPObject *obj)
{
    return NPNFuncs.releaseobject(obj);
}



/*
    Functions to access script objects represented by NPObject.

    Calls to script objects are synchronous.  If a function returns a
    value, it will be supplied via the result NPVariant
    argument. Successful calls will return true, false will be
    returned in case of an error.
    
    Calls made from plugin code to script must be made from the thread
    on which the plugin was initialized.
*/
bool NPN_Invoke(NPP npp, NPObject *obj, NPIdentifier methodName, const NPVariant *args, unsigned argCount, NPVariant *result)
{
    return NPNFuncs.invoke(npp, obj, methodName, args, argCount, result);
}



bool NPN_InvokeDefault(NPP npp, NPObject *obj, const NPVariant *args, unsigned argCount, NPVariant *result)
{
    return NPNFuncs.invokeDefault(npp, obj, args, argCount, result);
}



bool NPN_Evaluate(NPP npp, NPObject *obj, NPString *script, NPVariant *result)
{
    return NPNFuncs.evaluate(npp, obj, script, result);
}



bool NPN_GetProperty(NPP npp, NPObject *obj, NPIdentifier propertyName, NPVariant *result)
{
    return NPNFuncs.getproperty(npp, obj, propertyName, result);
}



bool NPN_SetProperty(NPP npp, NPObject *obj, NPIdentifier propertyName, const NPVariant *value)
{
    return NPNFuncs.setproperty(npp, obj, propertyName, value);
}



bool NPN_HasProperty(NPP npp, NPObject *npobj, NPIdentifier propertyName)
{
    return NPNFuncs.hasproperty(npp, npobj, propertyName);
}



bool NPN_HasMethod(NPP npp, NPObject *npobj, NPIdentifier methodName)
{
    return NPNFuncs.hasmethod(npp, npobj, methodName);
}



bool NPN_RemoveProperty(NPP npp, NPObject *obj, NPIdentifier propertyName)
{
    return NPNFuncs.removeproperty(npp, obj, propertyName);
}



bool NPN_Enumerate(NPP npp, NPObject *npobj, NPIdentifier **identifier, uint32_t *count)
{
    return NPNFuncs.enumerate(npp, npobj, identifier, count);
}



bool NPN_Construct(NPP npp, NPObject* obj, const NPVariant *args, uint32_t argCount, NPVariant *result)
{
    return NPNFuncs.construct(npp, obj, args, argCount, result);
}





/*
    NPN_SetException may be called to trigger a script exception upon return
    from entry points into NPObjects.
*/
void NPN_SetException(NPObject *obj, const NPUTF8 *message)
{
    return NPNFuncs.setexception(obj, message);
}

void flaw_set_npn_func_table(NPNetscapeFuncs *browserFuncs)
{
    memcpy(&NPNFuncs, browserFuncs, sizeof(NPNetscapeFuncs));
}

