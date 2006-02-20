//#  jTreeStateConv.cc: 
//#
//#  Copyright (C) 2002-2005
//#  ASTRON (Netherlands Foundation for Research in Astronomy)
//#  P.O.Box 2, 7990 AA Dwingeloo, The Netherlands, seg@astron.nl
//#
//#  This program is free software; you can redistribute it and/or modify
//#  it under the terms of the GNU General Public License as published by
//#  the Free Software Foundation; either version 2 of the License, or
//#  (at your option) any later version.
//#
//#  This program is distributed in the hope that it will be useful,
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//#  GNU General Public License for more details.
//#
//#  You should have received a copy of the GNU General Public License
//#  along with this program; if not, write to the Free Software
//#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//#

//# Always #include <lofar_config.h> first!
#include <lofar_config.h>

//# Includes
#include <Common/LofarLogger.h>
#include <jni.h>
#include <jOTDB2/nl_astron_lofar_sas_otb_jotdb2_jTreeStateConv.h>
#include <OTDB/OTDBtypes.h>
#include <string>
#include <iostream>
#include <jOTDB2/nl_astron_lofar_sas_otb_jotdb2_jCommon.h>
#include <OTDB/TreeStateConv.h>

using namespace LOFAR::OTDB;
using namespace std;


static TreeStateConv* treeStateConv;

JNIEXPORT void JNICALL Java_nl_astron_lofar_sas_otb_jotdb2_jTreeStateConv_initTreeStateConv(JNIEnv *, jobject) {
  
  OTDBconn = getConnection();
  treeStateConv = new TreeStateConv(OTDBconn);
}

JNIEXPORT jlong JNICALL Java_nl_astron_lofar_sas_otb_jotdb2_jTreeStateConv_get__Ljava_lang_String_2(JNIEnv *env, jobject, jstring aConv) {
  
  const char* chars = env->GetStringUTFChars (aConv, 0);
  const string str (chars);
  
  long ret = treeStateConv->get (str);
  
  env->ReleaseStringUTFChars (aConv, chars);	     
  
  return ret;
}

JNIEXPORT jstring JNICALL Java_nl_astron_lofar_sas_otb_jotdb2_jTreeStateConv_get__J(JNIEnv *env, jobject, jlong aConv) {
  jstring jstr = env->NewStringUTF (treeStateConv->get(aConv).c_str());
  return jstr;
}

JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb2_jTreeStateConv_getTypes(JNIEnv *env, jobject) {

  
  // Construct java Map
  jobject result;
  jclass mapClass, longClass;
  jmethodID mapInit, put, longInit;
    
  mapClass = env->FindClass("java/util/HashMap");
  mapInit = env->GetMethodID(mapClass, "<init>", "()V");
  result = env->NewObject(mapClass, mapInit);

  longClass = env->FindClass("java/lang/Long");
  longInit = env->GetMethodID(longClass, "<init>", "(J)V");
  


  if ( env->ExceptionOccurred() )
    return 0;
    
  put= env->GetMethodID(mapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

  if ( env->ExceptionOccurred() )
    return 0;
 
  treeStateConv->top();
  do {
    treeState key;
    string value;

    if (treeStateConv->get(key, value)) {
      env->CallObjectMethod(result, put, env->NewObject(longClass,
							longInit,
							(jlong)key), 
							env->NewStringUTF(value.c_str()));

      if ( env->ExceptionOccurred() )
	return 0;
   }
  } while (treeStateConv->next());

  return result;

}

JNIEXPORT void JNICALL Java_nl_astron_lofar_sas_otb_jotdb2_jTreeStateConv_top(JNIEnv *, jobject) {
  treeStateConv->top();
}


JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb2_jTreeStateConv_next(JNIEnv *, jobject) {
  jboolean aBool=treeStateConv->next();
  return aBool;
}
