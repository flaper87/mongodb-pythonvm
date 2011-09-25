//
//  engine_python.cpp
//  mongodb
//
//  Created by Flavio Percoco Premoli on 01/09/11.
//  Copyright 2011 Flavio [FlaPer87] Percoco Premoli. All rights reserved.
//

/**
 
 Some things to keep in mind:
    * Functions have to be named otherwise a random name will be created
    * Functions have a local scope wich doesn't affect the global scope
    * If you want a variable to be global it's necessary to use the global keyword
    * Each operation requires the GIL to be acquired
*/

/*
 Needs extra work.
    * No mongo to python converter yet
    * Bson to python
    * function with args
*/

#include "Python.h"
#include "engine_python.h"

#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

#include "../util/pybson/_cbson.h";

namespace mongo {
    
    PythonVMImpl * PythonVM = 0;
    PyThreadState* PythonVMImpl::m_thread_state = 0;
    
    TSP_DECLARE(PyInterpreter, _interpreter)
    
    // ScopedGILLock
    inline ScopedGILLock::ScopedGILLock(PyInterpreter *intrp) {
        // Acquires global lock and
        // sets the tstate to the thread specific
        // intrepreter state.
        pyInterpreter = intrp;
        PyEval_AcquireThread(pyInterpreter->interpreter);
    }
    inline ScopedGILLock::~ScopedGILLock() {
        // Releases the lock and sets
        // the thread state to NULL
        PyEval_ReleaseThread(pyInterpreter->interpreter);
    }
    
    // PyInterpreter
    inline PyInterpreter::PyInterpreter() {
        
        PyEval_AcquireLock();
        if (PyThreadState_Get() != PythonVMImpl::m_thread_state)
            PyThreadState_Swap(PythonVMImpl::m_thread_state);

        interpreter = Py_NewInterpreter();
        
        if (!interpreter) {
            out() << "ERROR Couldn't create a new python interpreter:" << '\n';
            assert(false);
        }
        
        main_module = import("__main__");
        main_namespace = main_module.attr("__dict__");
        PyDict_SetItemString(main_namespace.ptr(), "__builtins__", PyEval_GetBuiltins());
        
        PyThreadState_Swap(NULL);
        PyEval_ReleaseLock();
    }
    
    inline PyInterpreter::~PyInterpreter() {
        Py_EndInterpreter(interpreter);
    }
    
    // PythonVMImpl
    PythonVMImpl::PythonVMImpl(const char *appServerPath) {
        if (Py_IsInitialized() == 0) {
            Py_InitializeEx(0); // Let's not register signals handlers
            PyEval_InitThreads();
            PythonVMImpl::m_thread_state = PyThreadState_Get();
            PyEval_ReleaseLock();
        }
    }
    
    PythonVMImpl::~PythonVMImpl() {
        Py_Finalize();
    }
    
    // scope
    PyInterpreter* PythonVMImpl::_ts(bool reset) {
        PyInterpreter* interpreter = _interpreter.get();
        if (!reset && interpreter)
            return interpreter;
        
        interpreter = new PyInterpreter();
        _interpreter.reset(interpreter);
        return interpreter;   
    }
    
    PyInterpreter* PythonVMImpl::scopeCreate() {
        return _ts();
    }
    
    PyInterpreter* PythonVMImpl::scopeReset() {
        return _ts(true);
    }
    
    void PythonVMImpl::scopeFree() {
        Py_EndInterpreter(_ts()->interpreter);
    }
    
    void PythonVMImpl::scopeInit(const BSONObj *obj) {
        
    }
    
    int PythonVMImpl::invoke(ScriptingFunction function) {
        ScopedGILLock lock = ScopedGILLock(_ts());
        object  f = _funcs[function - 1];
        
        try {
            f();
        } catch(error_already_set const &) {
            PyErr_Clear();
            return 1;
        }
        
        return 0;
    }
    
    int PythonVMImpl::invoke(ScriptingFunction function, const BSONObj& args ) {
        ScopedGILLock lock = ScopedGILLock(_ts());
        object  f = _funcs[function - 1];
        
        dict kwargs = dict();
        
/*        BSONObjIterator it( BSONObj );
        while ( i.more() ) {
            BSONElement next = it.next();
            args[i] = mongoToV8Element( next, readOnlyArgs );
        }
       
        f(detail::args_proxy(tuple()),
          detail::kwds_proxy(kwargs)); */ 
        return 0;
    }
    
    // VM Getters
    double PythonVMImpl::scopeGetNumber( const char * field ) {
        ScopedGILLock lock = ScopedGILLock(_ts());
        
        return extract<double>(PythonVM->_ts()->main_namespace[field]);
    }
    
    bool PythonVMImpl::scopeGetBoolean(const char * field ) {
        ScopedGILLock lock = ScopedGILLock(_ts());
        
        return extract<bool>(PythonVM->_ts()->main_namespace[field]);
    }
    
    string PythonVMImpl::scopeGetString(const char * field ) {
        ScopedGILLock lock = ScopedGILLock(_ts());
        
        return extract<string>(PythonVM->_ts()->main_namespace[field]);
    }
    
    BSONObj PythonVMImpl::scopeGetObject(const char * field ) {
        
        return *(new BSONObj());
    }
    
    char PythonVMImpl::scopeGetType(const char * field) {
        char a;
        return a;
    }
    
    // VM Setters
    int PythonVMImpl::scopeSetNumber(const char * field , double val ) {
        ScopedGILLock lock = ScopedGILLock(_ts());
        
        _ts()->main_namespace[field] = object(val);
        return 0;
    }
    
    int PythonVMImpl::scopeSetString(const char * field , const char * val ) {
        ScopedGILLock lock = ScopedGILLock(_ts());
        
        _ts()->main_namespace[field] = object(val);
        return 0;
    }
    
    void PythonVMImpl::scopeSetObject(const char * field , const BSONObj * obj ) {
        //ScopedGILLock lock = ScopedGILLock(_ts());
        
        //_ts()->main_namespace[field] = object(val);
    }
    
    void PythonVMImpl::scopeSetElement(const char * field , const BSONElement * e ) {
    }
    
    void PythonVMImpl::scopeSetBoolean(const char * field , bool val ) {
        ScopedGILLock lock = ScopedGILLock(_ts());
        
        _ts()->main_namespace[field] = object(val);
    }
    
    void PythonVMImpl::scopeSetThis( const BSONObj * obj ) {
        
    }
    
    // PythonVM other
    
    ScriptingFunction PythonVMImpl::functionCreate( const char * code ) {
        ScopedGILLock lock = ScopedGILLock(_ts());
        
        string fname;
        string scode(code);
        
        if (!boost::algorithm::starts_with(scode, "def ")) {
            boost::format fFmt("def %1%():\n    %2%\n");
            
            fname = "f" + OID::gen().toString(); 
            fFmt % fname;
            fFmt % boost::algorithm::replace_all_copy(scode, "\n", "\n    ");
            scode = fFmt.str();
        } else {
            size_t pos = scode.find("(");
            fname = scode.substr(4, pos-4);
        }
        
        PyObject *pyCo = Py_CompileString (scode.c_str(), "<input>", Py_single_input);
        
        if (!pyCo || PyErr_Occurred()) {
            PyErr_Clear();
            return 0;
        }

        object ns = _ts()->main_namespace;
        boost::python::object objectCompiled;
        
        PyObject* pyObjFunc = PyFunction_New(pyCo, ns.ptr());
        
        if (!pyObjFunc || PyErr_Occurred()) {
            Py_XDECREF(pyCo);
            PyErr_Clear();
            return 0;
        }
        
        PyObject_CallFunctionObjArgs(pyObjFunc, NULL); /* Needed to define the function in the namespace (globals) */
        
        Py_XDECREF(pyCo);
        Py_XDECREF(pyObjFunc);
        
        //object newFunc = object(handle<>(pyObjFunc));
        object newFunc(ns[fname]);
        int num = _funcs.size() + 1;
        _funcs.push_back( newFunc );
        return num;
    }
    
    Scope * PythonVMImpl::createScope() {
        return new PythonScope();
    }
    
    void PythonVMImpl::setup() {
        if ( ! PythonVM ) {
            PythonVM = new PythonVMImpl();
            //globalScriptEngine = PythonVM;
        }
    }
    
    void PythonVMImpl::runTest() {
        out() << "Pasa" << endl;
    }
    
    // PythonScope
    PythonScope::PythonScope() {
        s = PythonVM->scopeCreate();
    }
    
    PythonScope::~PythonScope() {
        PythonVM->scopeFree();
        s = 0;
    }
    void PythonScope::reset() {
        PythonVM->scopeReset();
    }
    
    void PythonScope::init( const BSONObj * o ) {
        PythonVM->scopeInit( o );
    }
    
    void PythonScope::localConnect( const char * dbName ) {
        setString("$client", dbName );
    }
    
    // Scope Getters
    double PythonScope::getNumber(const char *field) {
        return PythonVM->scopeGetNumber(field);
    }
    
    string PythonScope::getString(const char *field) {
        return PythonVM->scopeGetString(field);
    }
    
    bool PythonScope::getBoolean(const char *field) {
        return PythonVM->scopeGetBoolean(field);
    }
    
    BSONObj PythonScope::getObject(const char *field ) {
        return PythonVM->scopeGetObject(field);
    }
    
    int PythonScope::type(const char *field ) {
        return PythonVM->scopeGetType(field);
    }
    
    // Scope Setters
    void PythonScope::setThis( const BSONObj * obj ) {
        PythonVM->scopeSetThis( obj );
    }
    
    void PythonScope::setNumber(const char *field, double val ) {
        PythonVM->scopeSetNumber(field,val);
    }
    
    void PythonScope::setString(const char *field, const char * val ) {
        PythonVM->scopeSetString(field,val);
    }
    
    void PythonScope::setObject(const char *field, const BSONObj& obj , bool readOnly ) {
        PythonVM->scopeSetObject(field,&obj);
    }
    
    void PythonScope::setElement(const char *field, const BSONElement& e ) {
        PythonVM->scopeSetElement(field, &e);
    }
    
    void PythonScope::setBoolean(const char *field, bool val ) {
        PythonVM->scopeSetBoolean(field, val);
    }
    
    void PythonScope::setFunction( const char *field , const char * code ) {
    }
    
    // Other
    ScriptingFunction PythonScope::_createFunction( const char * code ) {
        return PythonVM->functionCreate( code );
    }
    
    int PythonScope::invoke( ScriptingFunction function , const BSONObj& args ) {
        //setObject( "args" , args , true );
        return PythonVM->invoke(function, args);
    }
    
    int PythonScope::invoke(ScriptingFunction func , const BSONObj* args, const BSONObj* recv, int timeoutMs, bool ignoreReturn, bool readOnlyArgs, bool readOnlyRecv) {
        return 0;
    }
    
    bool PythonScope::exec(const StringData& code , const string& name , bool printResult , bool reportError , bool assertOnError, int timeoutMs) {
        return true;
    }
    
    void PythonScope::injectNative( const char *field, NativeFunction func, void* data ) {
    }
    
    void PythonScope::rename( const char * from , const char * to ) {
    }
    
    void PythonScope::gc() {
    }
    
    void PythonScope::externalSetup() {
    }
    
    string PythonScope::getError() {
        return getString( "error" );
    }
    
    // PyVmTest
    PyVmTest::~PyVmTest() {
        
    }
    
    void PyVmTest::run() {
        const int debug = 0;
        
        PythonVMImpl::setup();
            
        PythonVMImpl& PythonVM = *mongo::PythonVM;
        
        PythonVM.scopeCreate();
        
        if ( debug ) out() << "got scope" << endl;
        
        
        ScriptingFunction func1 = PythonVM.functionCreate("def greet():\n   return True\n");

        assert( PythonVM.invoke(func1) == 0);
        
        if ( debug ) out() << "func2 start" << endl;
        ScriptingFunction func2 = PythonVM.functionCreate( "def globaltest():\n    global z\n    z = True" );

        assert( ! PythonVM.invoke( func2 ) );
        assert( PythonVM.scopeGetBoolean( "z" ) );
        if ( debug ) out() << "func2 done" << endl;
        
        if ( debug ) out() << "func3 start" << endl;
        assert( PythonVM.scopeSetNumber( "mynumber" , (double)5 ) == 0 );
        ScriptingFunction func3 = PythonVM.functionCreate( "assert mynumber != 5" );
        assert( func3 );
        assert(PythonVM.invoke( func3 ) == 1 );
        if ( debug ) out() << "func3 done" << endl;
        
        if ( debug ) out() << "func4 start" << endl;
        assert( PythonVM.scopeSetString( "mystring" , "This is my string" ) == 0 );
        ScriptingFunction func4 = PythonVM.functionCreate( "assert mystring == \"This is my string\"" );
        assert( func4 );
        assert(PythonVM.invoke( func4 ) == 0 );
        if ( debug ) out() << "func4 done" << endl;

        
        
        /*
        if ( debug ) out() << "going to get object" << endl;
        BSONObj obj = JavaJS.scopeGetObject( scope , "abc" );
        if ( debug ) out() << "done getting object" << endl;
        
        if ( debug ) {
            out() << "obj : " << obj.toString() << endl;
        }
        
        
        
        {
            time_t start = time(0);
            for ( int i=0; i<5000; i++ ) {
                JavaJS.scopeSetObject( scope , "obj" , &obj );
            }
            time_t end = time(0);
            
            if ( debug )
                out() << "time : " << (unsigned) ( end - start ) << endl;
        }
        
        if ( debug ) out() << "func4 start" << endl;
        JavaJS.scopeSetObject( scope , "obj" , &obj );
        if ( debug ) out() << "\t here 1" << endl;
        jlong func4 = JavaJS.functionCreate( "tojson( obj );" );
        if ( debug ) out() << "\t here 2" << endl;
        jassert( ! JavaJS.invoke( scope , func4 ) );
        if ( debug ) out() << "func4 end" << endl;
        */

        /*
        if ( debug ) out() << "func6 start" << endl;
        for ( int i=0; i<100; i++ ) {
            double val = i + 5;
            JavaJS.scopeSetNumber( scope , "zzz" , val );
            jlong func6 = JavaJS.functionCreate( " xxx = zzz; " );
            jassert( ! JavaJS.invoke( scope , func6 ) );
            double n = JavaJS.scopeGetNumber( scope , "xxx" );
            jassert( val == n );
        }
        if ( debug ) out() << "func6 done" << endl;
        
        jlong func7 = JavaJS.functionCreate( "return 11;" );
        jassert( ! JavaJS.invoke( scope , func7 ) );
        assert( 11 == JavaJS.scopeGetNumber( scope , "return" ) );
        
        scope = JavaJS.scopeCreate();
        jlong func8 = JavaJS.functionCreate( "function(){ return 12; }" );
        jassert( ! JavaJS.invoke( scope , func8 ) );
        assert( 12 == JavaJS.scopeGetNumber( scope , "return" ) );
        */
        
    }

}