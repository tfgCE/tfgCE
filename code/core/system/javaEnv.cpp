#include "javaEnv.h"

#ifdef AN_ANDROID

#include "..\concurrency\spinLock.h"
#include "..\concurrency\scopedSpinLock.h"
#include "..\concurrency\threadSystemUtils.h"
#include "..\containers\arrayStatic.h"

#include "android\androidApp.h"

//

using namespace System;

//

JNIEnv* JavaEnv::get_env()
{
#ifdef AN_ANDROID
	auto* vm = ::System::Android::App::get().activityVM;
#else
#error implement!
#endif
	JNIEnv* env = nullptr;
	jint getEnvResult = vm->GetEnv((void**)&env, JNI_VERSION_1_6);
	if (getEnvResult == JNI_EDETACHED)
	{
		vm->AttachCurrentThread(&env, NULL);
	}
	return env;
}

void JavaEnv::drop_env()
{
#ifdef AN_ANDROID
	auto* vm = ::System::Android::App::get().activityVM;
#else
#error implement!
#endif
	vm->DetachCurrentThread();
}

jclass JavaEnv::find_class(JNIEnv* env, char const * _className)
{
#ifdef AN_ANDROID
	jobject nativeActivity = ::System::Android::App::get().get_activity()->clazz;
#else
#error implement!
#endif
	jclass acl = env->GetObjectClass(nativeActivity);
	jmethodID method_getClassLoader = env->GetMethodID(acl, "getClassLoader", "()Ljava/lang/ClassLoader;");
	jobject object_classLoader = env->CallObjectMethod(nativeActivity, method_getClassLoader);
	jclass class_classLoader = env->FindClass("java/lang/ClassLoader");
	jmethodID method_findClass = env->GetMethodID(class_classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
	jstring strClassName = env->NewStringUTF(_className);
	jclass foundClass = (jclass)(env->CallObjectMethod(object_classLoader, method_findClass, strClassName));
	env->DeleteLocalRef(strClassName);
	return foundClass;
}

void JavaEnv::make_global_ref(JNIEnv * env, jclass & _class)
{
	_class = reinterpret_cast<jclass>(env->NewGlobalRef(_class));
}

void JavaEnv::make_global_ref(JNIEnv* env, jobject & _object)
{
	_object = env->NewGlobalRef(_object);
}

#endif