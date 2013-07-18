/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance */

#ifndef _Included_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
#define _Included_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    initTreeMaintenance
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_initTreeMaintenance
  (JNIEnv *, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    loadMasterFile
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_loadMasterFile
  (JNIEnv *, jobject, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    loadComponentFile
 * Signature: (Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_loadComponentFile__Ljava_lang_String_2Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv *, jobject, jstring, jstring, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    loadComponentFile
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_loadComponentFile__Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    loadComponentFile
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_loadComponentFile__Ljava_lang_String_2
  (JNIEnv *, jobject, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getComponentList
 * Signature: (Ljava/lang/String;Z)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getComponentList__Ljava_lang_String_2Z
  (JNIEnv *, jobject, jstring, jboolean);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getComponentList
 * Signature: (Ljava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getComponentList__Ljava_lang_String_2
  (JNIEnv *, jobject, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getComponentList
 * Signature: ()Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getComponentList__
  (JNIEnv *, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getComponentNode
 * Signature: (I)Lnl/astron/lofar/sas/otb/jotdb3/jVICnodeDef;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getComponentNode
  (JNIEnv *, jobject, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getComponentParams
 * Signature: (I)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getComponentParams
  (JNIEnv *, jobject, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    saveComponentNode
 * Signature: (Lnl/astron/lofar/sas/otb/jotdb3/jVICnodeDef;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_saveComponentNode
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    isTopComponent
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_isTopComponent
  (JNIEnv *, jobject, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    deleteComponentNode
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_deleteComponentNode
  (JNIEnv *, jobject, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getFullComponentName
 * Signature: (Lnl/astron/lofar/sas/otb/jotdb3/jVICnodeDef;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getFullComponentName
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    buildTemplateTree
 * Signature: (IS)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_buildTemplateTree
  (JNIEnv *, jobject, jint, jshort);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    newTemplateTree
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_newTemplateTree
  (JNIEnv *, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    copyTemplateTree
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_copyTemplateTree
  (JNIEnv *, jobject, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    assignTemplateName
 * Signature: (ILjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_assignTemplateName
  (JNIEnv *, jobject, jint, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    assignProcessType
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_assignProcessType
  (JNIEnv *, jobject, jint, jstring, jstring, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getNode
 * Signature: (II)Lnl/astron/lofar/sas/otb/jotdb3/jOTDBnode;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getNode
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getParam
 * Signature: (II)Lnl/astron/lofar/sas/otb/jotdb3/jOTDBparam;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getParam__II
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getParam
 * Signature: (Lnl/astron/lofar/sas/otb/jotdb3/jOTDBnode;)Lnl/astron/lofar/sas/otb/jotdb3/jOTDBparam;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getParam__Lnl_astron_lofar_sas_otb_jotdb3_jOTDBnode_2
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    saveParam
 * Signature: (Lnl/astron/lofar/sas/otb/jotdb3/jOTDBparam;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_saveParam
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getItemList
 * Signature: (III)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getItemList__III
  (JNIEnv *, jobject, jint, jint, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getItemList
 * Signature: (ILjava/lang/String;)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getItemList__ILjava_lang_String_2
  (JNIEnv *, jobject, jint, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getItemList
 * Signature: (ILjava/lang/String;Z)Ljava/util/Vector;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getItemList__ILjava_lang_String_2Z
  (JNIEnv *, jobject, jint, jstring, jboolean);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    dupNode
 * Signature: (IIS)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_dupNode
  (JNIEnv *, jobject, jint, jint, jshort);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    addComponent
 * Signature: (IIILjava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_addComponent__IIILjava_lang_String_2
  (JNIEnv *, jobject, jint, jint, jint, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    addComponent
 * Signature: (III)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_addComponent__III
  (JNIEnv *, jobject, jint, jint, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    saveNode
 * Signature: (Lnl/astron/lofar/sas/otb/jotdb3/jOTDBnode;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_saveNode
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    saveNodeList
 * Signature: (Ljava/util/Vector;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_saveNodeList
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    deleteNode
 * Signature: (Lnl/astron/lofar/sas/otb/jotdb3/jOTDBnode;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_deleteNode
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    deleteNodeList
 * Signature: (Ljava/util/Vector;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_deleteNodeList
  (JNIEnv *, jobject, jobject);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    checkTreeConstraints
 * Signature: (II)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_checkTreeConstraints__II
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    checkTreeConstraints
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_checkTreeConstraints__I
  (JNIEnv *, jobject, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    instanciateTree
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_instanciateTree
  (JNIEnv *, jobject, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    pruneTree
 * Signature: (IS)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_pruneTree
  (JNIEnv *, jobject, jint, jshort);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    exportTree
 * Signature: (IILjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_exportTree
  (JNIEnv *, jobject, jint, jint, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    exportResultTree
 * Signature: (IILjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_exportResultTree
  (JNIEnv *, jobject, jint, jint, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    exportMetadata
 * Signature: (ILjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_exportMetadata__ILjava_lang_String_2
  (JNIEnv *, jobject, jint, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    exportMetadata
 * Signature: (ILjava/lang/String;Z)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_exportMetadata__ILjava_lang_String_2Z
  (JNIEnv *, jobject, jint, jstring, jboolean);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    deleteTree
 * Signature: (I)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_deleteTree
  (JNIEnv *, jobject, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    getTopNode
 * Signature: (I)Lnl/astron/lofar/sas/otb/jotdb3/jOTDBnode;
 */
JNIEXPORT jobject JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_getTopNode
  (JNIEnv *, jobject, jint);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    setMomInfo
 * Signature: (IIILjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_setMomInfo
  (JNIEnv *, jobject, jint, jint, jint, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    setClassification
 * Signature: (IS)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_setClassification
  (JNIEnv *, jobject, jint, jshort);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    setTreeState
 * Signature: (IS)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_setTreeState__IS
  (JNIEnv *, jobject, jint, jshort);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    setTreeState
 * Signature: (ISZ)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_setTreeState__ISZ
  (JNIEnv *, jobject, jint, jshort, jboolean);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    setDescription
 * Signature: (ILjava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_setDescription
  (JNIEnv *, jobject, jint, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    setSchedule
 * Signature: (ILjava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_setSchedule__ILjava_lang_String_2Ljava_lang_String_2
  (JNIEnv *, jobject, jint, jstring, jstring);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    setSchedule
 * Signature: (ILjava/lang/String;Ljava/lang/String;Z)Z
 */
JNIEXPORT jboolean JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_setSchedule__ILjava_lang_String_2Ljava_lang_String_2Z
  (JNIEnv *, jobject, jint, jstring, jstring, jboolean);

/*
 * Class:     nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance
 * Method:    errorMsg
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_nl_astron_lofar_sas_otb_jotdb3_jTreeMaintenance_errorMsg
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
