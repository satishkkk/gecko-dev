/* Copyright (c) 2000-2001 ActiveState Tool Corporation.
   See the file LICENSE.txt for licensing information. */

//
// This code is part of the XPCOM extensions for Python.
//
// Written May 2000 by Mark Hammond.
//
// Based heavily on the Python COM support, which is
// (c) Mark Hammond and Greg Stein.
//
// (c) 2000, ActiveState corp.

// Unfortunately, we can not use an XPConnect object for
// the nsiModule and nsiComponentLoader interfaces.
//  As XPCOM shuts down, it shuts down the interface manager before 
// it releases all the modules.  This is a bit of a problem for 
// us, as it means we can't get runtime info on the interface at shutdown time.

#include "PyXPCOM_std.h"
#include <nsIModule.h>
#include <nsIComponentLoader.h>

#ifdef XP_WIN
// Can only assume dynamic loading on Windows.
#define LOADER_LINKS_WITH_PYTHON

#endif // XP_WIN

extern void PyXPCOM_InterpreterState_Ensure();

////////////////////////////////////////////////////////////
// This is the main entry point called by the Python component
// loader.
extern "C" NS_EXPORT nsresult PyXPCOM_NSGetModule(nsIComponentManager *servMgr,
                                          nsIFile* location,
                                          nsIModule** result)
{
	NS_PRECONDITION(result!=NULL, "null result pointer in PyXPCOM_NSGetModule!");
	NS_PRECONDITION(location!=NULL, "null nsIFile pointer in PyXPCOM_NSGetModule!");
	NS_PRECONDITION(servMgr!=NULL, "null servMgr pointer in PyXPCOM_NSGetModule!");
#ifndef LOADER_LINKS_WITH_PYTHON
	if (!Py_IsInitialized()) {
		Py_Initialize();
		if (!Py_IsInitialized()) {
			PyXPCOM_LogError("Python initialization failed!\n");
			return NS_ERROR_FAILURE;
		}
		PyEval_InitThreads();
		PyXPCOM_InterpreterState_Ensure();
		PyEval_SaveThread();
	}
#endif // LOADER_LINKS_WITH_PYTHON	
	CEnterLeavePython _celp;
	PyObject *func = NULL;
	PyObject *obServMgr = NULL;
	PyObject *obLocation = NULL;
	PyObject *wrap_ret = NULL;
	PyObject *args = NULL;
	PyObject *mod = PyImport_ImportModule("xpcom.server");
	if (!mod) goto done;
	func = PyObject_GetAttrString(mod, "NS_GetModule");
	if (func==NULL) goto done;
	obServMgr = Py_nsISupports::PyObjectFromInterface(servMgr, NS_GET_IID(nsIComponentManager), PR_TRUE);
	if (obServMgr==NULL) goto done;
	obLocation = Py_nsISupports::PyObjectFromInterface(location, NS_GET_IID(nsIFile), PR_TRUE);
	if (obLocation==NULL) goto done;
	args = Py_BuildValue("OO", obServMgr, obLocation);
	if (args==NULL) goto done;
	wrap_ret = PyEval_CallObject(func, args);
	if (wrap_ret==NULL) goto done;
	Py_nsISupports::InterfaceFromPyObject(wrap_ret, NS_GET_IID(nsIModule), (nsISupports **)result, PR_FALSE, PR_FALSE);
done:
	nsresult nr = NS_OK;
	if (PyErr_Occurred()) {
		PyXPCOM_LogError("Obtaining the module object from Python failed.\n");
		nr = PyXPCOM_SetCOMErrorFromPyException();
	}
	Py_XDECREF(func);
	Py_XDECREF(obServMgr);
	Py_XDECREF(obLocation);
	Py_XDECREF(wrap_ret);
	Py_XDECREF(mod);
	Py_XDECREF(args);
	return nr;
}

class PyG_nsIModule : public PyG_Base, public nsIModule
{
public:
	PyG_nsIModule(PyObject *instance) : PyG_Base(instance, NS_GET_IID(nsIModule)) {;}
	PYGATEWAY_BASE_SUPPORT(nsIModule, PyG_Base);

	NS_IMETHOD GetClassObject(nsIComponentManager *aCompMgr, const nsCID & aClass, const nsIID & aIID, void * *result);
	NS_IMETHOD RegisterSelf(nsIComponentManager *aCompMgr, nsIFile *location, const char *registryLocation, const char *componentType);
	NS_IMETHOD UnregisterSelf(nsIComponentManager *aCompMgr, nsIFile *location, const char *registryLocation);
	NS_IMETHOD CanUnload(nsIComponentManager *aCompMgr, PRBool *_retval);
};

PyG_Base *MakePyG_nsIModule(PyObject *instance)
{
	return new PyG_nsIModule(instance);
}


// Create a factory object for creating instances of aClass.
NS_IMETHODIMP
PyG_nsIModule::GetClassObject(nsIComponentManager *aCompMgr,
                                const nsCID& aClass,
                                const nsIID& aIID,
                                void** r_classObj)
{
	NS_PRECONDITION(r_classObj, "null pointer");
	*r_classObj = nsnull;
	CEnterLeavePython _celp;
	PyObject *cm = MakeInterfaceParam(aCompMgr, &NS_GET_IID(nsIComponentManager));
	PyObject *iid = Py_nsIID::PyObjectFromIID(aIID);
	PyObject *clsid = Py_nsIID::PyObjectFromIID(aClass);
	const char *methodName = "getClassObject";
	PyObject *ret = NULL;
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret, "OOO", cm, clsid, iid);
	Py_XDECREF(cm);
	Py_XDECREF(iid);
	Py_XDECREF(clsid);
	if (NS_SUCCEEDED(nr)) {
		nr = Py_nsISupports::InterfaceFromPyObject(ret, aIID, (nsISupports **)r_classObj, PR_FALSE);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
	}
	if (NS_FAILED(nr)) {
		NS_ABORT_IF_FALSE(*r_classObj==NULL, "returning error result with an interface - probable leak!");
	}
	Py_XDECREF(ret);
	return nr;
}

NS_IMETHODIMP
PyG_nsIModule::RegisterSelf(nsIComponentManager *aCompMgr,
                              nsIFile* aPath,
                              const char* registryLocation,
                              const char* componentType)
{
	NS_PRECONDITION(aCompMgr, "null pointer");
	NS_PRECONDITION(aPath, "null pointer");
	CEnterLeavePython _celp;
	PyObject *cm = MakeInterfaceParam(aCompMgr, &NS_GET_IID(nsIComponentManager));
	PyObject *path = MakeInterfaceParam(aPath, &NS_GET_IID(nsIFile));
	const char *methodName = "registerSelf";
	nsresult nr = InvokeNativeViaPolicy(methodName, NULL, "OOzz", cm, path, registryLocation, componentType);
	Py_XDECREF(cm);
	Py_XDECREF(path);
	return nr;
}

NS_IMETHODIMP
PyG_nsIModule::UnregisterSelf(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char* registryLocation)
{
	NS_PRECONDITION(aCompMgr, "null pointer");
	NS_PRECONDITION(aPath, "null pointer");
	CEnterLeavePython _celp;
	PyObject *cm = MakeInterfaceParam(aCompMgr, &NS_GET_IID(nsIComponentManager));
	PyObject *path = MakeInterfaceParam(aPath, &NS_GET_IID(nsIFile));
	const char *methodName = "unregisterSelf";
	nsresult nr = InvokeNativeViaPolicy(methodName, NULL, "OOz", cm, path, registryLocation);
	Py_XDECREF(cm);
	Py_XDECREF(path);
	return nr;
}

NS_IMETHODIMP
PyG_nsIModule::CanUnload(nsIComponentManager *aCompMgr, PRBool *okToUnload)
{
	NS_PRECONDITION(aCompMgr, "null pointer");
	NS_PRECONDITION(okToUnload, "null pointer");
	CEnterLeavePython _celp;
	PyObject *cm = MakeInterfaceParam(aCompMgr, &NS_GET_IID(nsIComponentManager));
	const char *methodName = "canUnload";
	PyObject *ret = NULL;
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret, "O", cm);
	Py_XDECREF(cm);
	if (NS_SUCCEEDED(nr)) {
		*okToUnload = PyInt_AsLong(ret);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
	}
	Py_XDECREF(ret);
	return nr;
}

///////////////////////////////////////////////////////////////////////////////////

class PyG_nsIComponentLoader : public PyG_Base, public nsIComponentLoader
{
public:
	PyG_nsIComponentLoader(PyObject *instance) : PyG_Base(instance, NS_GET_IID(nsIComponentLoader)) {;}
	PYGATEWAY_BASE_SUPPORT(nsIComponentLoader, PyG_Base);

	NS_DECL_NSICOMPONENTLOADER
};

PyG_Base *MakePyG_nsIComponentLoader(PyObject *instance)
{
	return new PyG_nsIComponentLoader(instance);
}

/* nsIFactory getFactory (in nsIIDRef aCID, in string aLocation, in string aType); */
NS_IMETHODIMP PyG_nsIComponentLoader::GetFactory(const nsIID & aCID, const char *aLocation, const char *aType, nsIFactory **_retval)
{
	CEnterLeavePython _celp;
	const char *methodName = "getFactory";
	PyObject *iid = Py_nsIID::PyObjectFromIID(aCID);
	PyObject *ret = NULL;
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret, "Ozz", 
				iid,
				aLocation,
				aType);
	Py_XDECREF(iid);
	if (NS_SUCCEEDED(nr)) {
		Py_nsISupports::InterfaceFromPyObject(ret, NS_GET_IID(nsIFactory), (nsISupports **)_retval, PR_FALSE);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
	}
	Py_XDECREF(ret);
	return nr;
}

/* void init (in nsIComponentManager aCompMgr, in nsISupports aRegistry); */
NS_IMETHODIMP PyG_nsIComponentLoader::Init(nsIComponentManager *aCompMgr, nsISupports *aRegistry)
{
	CEnterLeavePython _celp;
	const char *methodName = "init";
	PyObject *c = MakeInterfaceParam(aCompMgr, &NS_GET_IID(nsIComponentManager));
	PyObject *r = MakeInterfaceParam(aRegistry, &NS_GET_IID(nsISupports));
	nsresult nr = InvokeNativeViaPolicy(methodName, NULL, "OO", c, r);
	Py_XDECREF(c);
	Py_XDECREF(r);
	return nr;
}

/* void onRegister (in nsIIDRef aCID, in string aType, in string aClassName, in string aContractID, in string aLocation, in boolean aReplace, in boolean aPersist); */
NS_IMETHODIMP PyG_nsIComponentLoader::OnRegister(const nsIID & aCID, const char *aType, const char *aClassName, const char *aContractID, const char *aLocation, PRBool aReplace, PRBool aPersist)
{
	CEnterLeavePython _celp;
	const char *methodName = "onRegister";
	PyObject *iid = Py_nsIID::PyObjectFromIID(aCID);
	nsresult nr = InvokeNativeViaPolicy(methodName, NULL, "Ossssii", 
				iid,
				aType,
				aClassName, 
				aContractID, 
				aLocation, 
				aReplace,
				aPersist);
	Py_XDECREF(iid);
	return nr;
}

/* void autoRegisterComponents (in long aWhen, in nsIFile aDirectory); */
NS_IMETHODIMP PyG_nsIComponentLoader::AutoRegisterComponents(PRInt32 aWhen, nsIFile *aDirectory)
{
	CEnterLeavePython _celp;
	const char *methodName = "autoRegisterComponents";
	PyObject *c = MakeInterfaceParam(aDirectory, &NS_GET_IID(nsIFile));
	nsresult nr = InvokeNativeViaPolicy(methodName, NULL, "iO", aWhen, c);
	Py_XDECREF(c);
	return nr;
}

/* boolean autoRegisterComponent (in long aWhen, in nsIFile aComponent); */
NS_IMETHODIMP PyG_nsIComponentLoader::AutoRegisterComponent(PRInt32 aWhen, nsIFile *aComponent, PRBool *_retval)
{
	CEnterLeavePython _celp;
	const char *methodName = "autoRegisterComponent";
	PyObject *ret = NULL;
	PyObject *c = MakeInterfaceParam(aComponent, &NS_GET_IID(nsIFile));
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret, "iO", aWhen, c);
	Py_XDECREF(c);
	if (NS_SUCCEEDED(nr)) {
		*_retval = PyInt_AsLong(ret);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
	}
	Py_XDECREF(ret);
	return nr;
}

/* boolean autoUnregisterComponent (in long aWhen, in nsIFile aComponent); */
NS_IMETHODIMP PyG_nsIComponentLoader::AutoUnregisterComponent(PRInt32 aWhen, nsIFile *aComponent, PRBool *_retval)
{
	CEnterLeavePython _celp;
	const char *methodName = "autoUnregisterComponent";
	PyObject *ret = NULL;
	PyObject *c = MakeInterfaceParam(aComponent, &NS_GET_IID(nsIFile));
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret, "iO", aWhen, c);
	Py_XDECREF(c);
	if (NS_SUCCEEDED(nr)) {
		*_retval = PyInt_AsLong(ret);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
	}
	Py_XDECREF(ret);
	return nr;
}

/* boolean registerDeferredComponents (in long aWhen); */
NS_IMETHODIMP PyG_nsIComponentLoader::RegisterDeferredComponents(PRInt32 aWhen, PRBool *_retval)
{
	CEnterLeavePython _celp;
	const char *methodName = "registerDeferredComponents";
	PyObject *ret = NULL;
	nsresult nr = InvokeNativeViaPolicy(methodName, &ret, "i", aWhen);
	if (NS_SUCCEEDED(nr)) {
		*_retval = PyInt_AsLong(ret);
		if (PyErr_Occurred())
			nr = HandleNativeGatewayError(methodName);
	}
	Py_XDECREF(ret);
	return nr;
}

/* void unloadAll (in long aWhen); */
NS_IMETHODIMP PyG_nsIComponentLoader::UnloadAll(PRInt32 aWhen)
{
	CEnterLeavePython _celp;
	const char *methodName = "unloadAll";
	return InvokeNativeViaPolicy(methodName, NULL, "i", aWhen);
}


