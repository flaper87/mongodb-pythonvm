//
//  engine_python.h
//  mongodb
//
//  Created by Flavio Percoco Premoli on 05/09/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#pragma once

#include "../pch.h"

#include <boost/python.hpp>

#include <vector>
#include <errno.h>
#include <sys/types.h>

#include "../db/jsobj.h"
#include "../util/unittest.h"
#include "../util/concurrency/threadlocal.h" //TODO

#include "engine.h"

using namespace boost::python;

namespace mongo {
    
    class BSONObj;
    class PythonVMImpl;
    class PyInterpreter;
    
    class ScopedGILLock {
    public:
        ScopedGILLock(PyInterpreter *intrp);
        ~ScopedGILLock();
        
    private:
        PyInterpreter* pyInterpreter;
    };
    
    class PyInterpreter {
    public:
        PyInterpreter();
        ~PyInterpreter();

        object main_module;
        object main_namespace;
        PyThreadState * interpreter;
    };
    
    class PythonVMImpl : public ScriptEngine {
    public:
        PythonVMImpl(const char * = 0);
        ~PythonVMImpl();
        
        PyInterpreter *scopeCreate();
        PyInterpreter *scopeReset();
        void scopeFree();
        
        int invoke(ScriptingFunction function);
        
        int invoke(ScriptingFunction function, const BSONObj& args );
        
        void printException();
        
        //void run( const char * js );
        void scopeInit(const BSONObj * obj);
        
        Scope * createScope();
        
        static void setup();
        
        double scopeGetNumber(const char * field );
        string scopeGetString(const char * field );
        bool scopeGetBoolean(const char * field );
        BSONObj scopeGetObject(const char * field );
        char scopeGetType(const char * field );
        
        int scopeSetNumber(const char * field , double val );
        int scopeSetString(const char * field , const char * val );
        void scopeSetObject(const char * field , const BSONObj * obj );
        void scopeSetBoolean(const char * field , bool val );
        void scopeSetThis( const BSONObj * obj );
        void scopeSetElement(const char * field , const BSONElement * e );
        
        ScriptingFunction functionCreate( const char * code );
        
        virtual bool utf8Ok() const { return false;}
        
        void runTest();
        static PyThreadState* m_thread_state;
        
    private:
        vector<object> _funcs;
        
        PyInterpreter* _ts(bool reset = false);
        TSP_DEFINE(PyInterpreter, _interpreter)
    };
        
    extern PythonVMImpl *PythonVM;
    
    class PythonScope : public Scope {
    public:
        PythonScope();
        virtual ~PythonScope();
        
        void reset();
        
        void init( const BSONObj * o );
        
        void localConnect( const char * dbName );
        
        double getNumber(const char *field);
        
        string getString(const char *field);
        
        bool getBoolean(const char *field);
        
        BSONObj getObject(const char *field );
        
        int type(const char *field );
        
        void setThis( const BSONObj * obj );
        
        void setNumber(const char *field, double val );
        
        void setString(const char *field, const char * val );
        
        void setObject(const char *field, const BSONObj& obj , bool readOnly );
        
        void setBoolean(const char *field, bool val );
        
        void setElement( const char *field , const BSONElement& e );
        
        void setFunction( const char *field , const char * code );
        
        ScriptingFunction _createFunction( const char * code );
        
        int invoke( ScriptingFunction func, const BSONObj& args );
        
        int invoke( ScriptingFunction func , const BSONObj* args, const BSONObj* recv, int timeoutMs = 0 , bool ignoreReturn = false, bool readOnlyArgs = false, bool readOnlyRecv = false  );
        
        bool exec(const StringData& code , const string& name , bool printResult , bool reportError , bool assertOnError, int timeoutMs = 0);
        
        void injectNative( const char *field, NativeFunction func, void* data = 0 );
        
        void gc();
        
        void externalSetup();
        
        void rename( const char * from , const char * to );

        
        string getError();
        
        PyInterpreter * s;
    };
    
    class PyVmTest : public UnitTest {
    public:
        ~PyVmTest();
        
        void run();
        
    } pyVmTest;
    
} // namespace mongo
