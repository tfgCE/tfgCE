#include "viveport.h"

#include "..\..\concurrency\thread.h"
#include "..\..\concurrency\scopedSpinLock.h"
#include "..\..\system\core.h"

#ifdef AN_VIVEPORT
	#ifdef AN_ANDROID
		#include "..\..\system\android\androidApp.h"
		#include "..\..\system\javaEnv.h"
	#else
		#include "viveport_api.h"
		#include "viveport_ext_api.h"
	#endif

	#include <string>
#endif

//

using namespace GameEnvironment;

//

struct GameEnvironment::ViveportImpl
{
	static GameEnvironment::ViveportImpl* s_impl;

	Concurrency::SpinLock stateLock;
	Optional<bool> isInitialised;
	Optional<bool> isLicenseOk;

#ifdef AN_ANDROID
	jclass class_activity;
	jclass class_viveportModule;

	jobject object_viveportModule;
	
	jmethodID method_getVersion;
	jmethodID method_init;
	jmethodID method_getLicense;
	jmethodID method_shutdown;
#endif

	ViveportImpl(tchar const* _viveportId, tchar const* _viveportKey)
	{
		an_assert(!s_impl);
		s_impl = this;

#ifdef AN_VIVEPORT
		viveportId = _viveportId;
		viveportKey = _viveportKey;
#ifdef AN_ANDROID
		s_impl->init_java();
#endif
#endif
	}

#ifdef AN_VIVEPORT
#ifdef AN_ANDROID
	// this should be called after the game is running, we shouldn't start earlier
	void init_java()
	{
		JNIEnv* env = System::JavaEnv::get_env();
		class_activity = System::JavaEnv::find_class(env, "com/voidroom/TeaForGodActivity");
		System::JavaEnv::make_global_ref(env, class_activity);

		jobject object_activity = System::Android::App::get().get_activity()->clazz;

		jmethodID method_getViveportModule = env->GetMethodID(class_activity, "getViveportModule", "()Lcom/voidroom/ViveportModule;");
		object_viveportModule = env->CallObjectMethod(object_activity, method_getViveportModule);
		System::JavaEnv::make_global_ref(env, object_viveportModule);

		class_viveportModule = System::JavaEnv::find_class(env, "com/voidroom/ViveportModule");
		System::JavaEnv::make_global_ref(env, class_viveportModule);

		method_getVersion = env->GetMethodID(class_viveportModule, "getVersion", "()Ljava/lang/String;");
		method_init = env->GetMethodID(class_viveportModule, "init", "(Ljava/lang/String;)I");
		method_getLicense = env->GetMethodID(class_viveportModule, "getLicense", "(Ljava/lang/String;Ljava/lang/String;)V");
		method_shutdown = env->GetMethodID(class_viveportModule, "shutdown", "()V");
	}
#endif
#endif

	~ViveportImpl()
	{
#ifdef AN_VIVEPORT
		shutdown_async();
#endif
		an_assert(s_impl == this);
		s_impl = nullptr;
	}

#ifdef AN_VIVEPORT
	String viveportId;
	String viveportKey;

	static void init_callback(int nResult);
	void init_async(); // will get license afterwards

#ifdef AN_ANDROID
	static void get_license_async_callback(int nResult);
#else
	static void get_license_async_callback(const char* _message, const char* _signature);
#endif
	void get_license_async();

	static void shutdown_callback(int nResult);
	void shutdown_async();
#endif
};

#ifdef AN_VIVEPORT
#ifdef AN_ANDROID
// created with:
//	javah -o vm.h com.voidroom.ViveportModule
// called from runtime\ARM64\ReleaseVive\Package\bin\classes
// "C" to keep function names as they are
extern "C" 
{
	JNIEXPORT void JNICALL Java_com_voidroom_ViveportModule_init_1callback(JNIEnv*, jobject, jint nResult) { ViveportImpl::init_callback((int)nResult); }
	JNIEXPORT void JNICALL Java_com_voidroom_ViveportModule_get_1license_1callback(JNIEnv*, jobject, jint nResult) { ViveportImpl::get_license_async_callback((int)nResult); }
	JNIEXPORT void JNICALL Java_com_voidroom_ViveportModule_shutdown_1callback(JNIEnv*, jobject, jint nResult) { ViveportImpl::shutdown_callback((int)nResult); }
}
#endif
#endif

GameEnvironment::ViveportImpl* GameEnvironment::ViveportImpl::s_impl = nullptr;

#ifdef AN_VIVEPORT
void GameEnvironment::ViveportImpl::init_callback(int nResult)
{
	if (! s_impl->isInitialised.is_set())
	{
		Concurrency::ScopedSpinLock lock(s_impl->stateLock);

		if (nResult == 0)
		{
			s_impl->isInitialised = true;
			output(TXT("[viveport] initialised"));
		}
		else
		{
			error(TXT("[viveport] failed to initialise"));
			s_impl->isInitialised = false;
		}
	}
	else
	{
		output(TXT("[viveport] initialise callback done"));
	}

	if (s_impl->isInitialised.get(false))
	{
		s_impl->get_license_async();
	}
}

void GameEnvironment::ViveportImpl::init_async()
{
#ifndef AN_ANDROID
	if (!ViveportAPI())
	{
		Concurrency::ScopedSpinLock lock(stateLock);
		error(TXT("[viveport] can't access api"));
		isInitialised = false;
		return;
	}
#endif
	{
		// do it on a separate thread (as it requires to run Vive)
		// we don't want to block this thread
		// and we should be checking if the system is okay further down the line
		Concurrency::Thread::start_off_system_thread([]()
			{
				{
					String version;
#ifdef AN_ANDROID
					{
						auto* env = System::JavaEnv::get_env();
						jstring javaVersion = (jstring)env->CallObjectMethod(s_impl->object_viveportModule, s_impl->method_getVersion);
						const char* nativeVersion = env->GetStringUTFChars(javaVersion, 0);
						version = String::from_char8_utf(nativeVersion);
						env->ReleaseStringUTFChars(javaVersion, nativeVersion);
					}
#else
					{
						auto* v = ViveportAPI()->Version();
						if (v)
						{
							version = String::from_char8(v);
						}
					}
#endif
					if (!version.is_empty())
					{
						output(TXT("[viveport] version: %S"), version.to_char());
					}
				}
				output(TXT("[viveport] initialise"));
#ifdef AN_ANDROID
				auto* env = System::JavaEnv::get_env();
				jstring javaAppId = env->NewStringUTF(s_impl->viveportId.to_char8_utf_array().get_data());
				int initResult = env->CallIntMethod(s_impl->object_viveportModule, s_impl->method_init, javaAppId);
				env->DeleteLocalRef(javaAppId);
#else
				int initResult = ViveportAPI()->Init(init_callback, s_impl->viveportId.to_char8_utf_array().get_data());
#endif
				if (initResult == 0)
				{
					Concurrency::ScopedSpinLock lock(s_impl->stateLock);
					s_impl->isInitialised = false;
					error(TXT("[viveport] could not initialise"));
				}
			});
	}
}

#ifdef AN_ANDROID
void GameEnvironment::ViveportImpl::get_license_async_callback(int nResult)
{
	Concurrency::ScopedSpinLock lock(s_impl->stateLock);

	if (nResult == 0)
	{
		output(TXT("[viveport] proper license received"));
	}
	else
	{
		error(TXT("[viveport] invalid license"));
	}
	s_impl->isLicenseOk = nResult == 0;
}
#else
void GameEnvironment::ViveportImpl::get_license_async_callback(const char* _message, const char* _signature)
{
	{
		Concurrency::ScopedSpinLock lock(s_impl->stateLock);
		bool verified = _message != nullptr && strlen(_message) > 0;

		if (!verified)
		{
			error(TXT("[viveport] no proper message for get license"));
			s_impl->isLicenseOk = false;
		}
		else
		{
			std::string message = std::string(_message);
			std::string signature = std::string(_signature);

			output(TXT("[viveport] message received:\n%S"), String::from_char8(_message).to_char());

			auto verificationResult = ViveportExtLicense()->VerifyMessage(
				s_impl->viveportId.to_char8_utf_array().get_data(),
				s_impl->viveportKey.to_char8_utf_array().get_data(),
				message.c_str(),
				signature.c_str()
			);

			if (verificationResult > 0)
			{
				output(TXT("[viveport] proper license received"));
				s_impl->isLicenseOk = true;
			}
			else
			{
				error(TXT("[viveport] invalid license"));
				s_impl->isLicenseOk = false;
			}
		}
	}
}
#endif

void GameEnvironment::ViveportImpl::get_license_async()
{
	output(TXT("[viveport] get license"));
#ifdef AN_ANDROID
	auto* env = System::JavaEnv::get_env();
	jstring javaAppId = env->NewStringUTF(s_impl->viveportId.to_char8_utf_array().get_data());
	jstring javaAppKey = env->NewStringUTF(s_impl->viveportKey.to_char8_utf_array().get_data());
	env->CallVoidMethod(s_impl->object_viveportModule, s_impl->method_getLicense, javaAppId, javaAppKey);
	env->DeleteLocalRef(javaAppKey);
	env->DeleteLocalRef(javaAppId);
#else
	ViveportAPI()->GetLicense(get_license_async_callback, viveportId.to_char8_utf_array().get_data(), viveportKey.to_char8_utf_array().get_data());
#endif
}

void GameEnvironment::ViveportImpl::shutdown_callback(int nResult)
{
}

void GameEnvironment::ViveportImpl::shutdown_async()
{
#ifdef AN_ANDROID
	auto* env = System::JavaEnv::get_env();
	env->CallIntMethod(s_impl->object_viveportModule, s_impl->method_shutdown);
#else
	ViveportAPI()->Shutdown(shutdown_callback);
#endif
}
#endif

//

void Viveport::init(tchar const* _viveportId, tchar const* _viveportKey)
{
	new Viveport(_viveportId, _viveportKey);
}

Viveport::Viveport(tchar const* _viveportId, tchar const* _viveportKey)
: impl(new ViveportImpl(_viveportId, _viveportKey))
{
#ifdef AN_VIVEPORT
	impl->init_async();
#endif
}

Viveport::~Viveport()
{
	delete impl;
}

Optional<bool> Viveport::is_ok() const
{
	Concurrency::ScopedSpinLock lock(impl->stateLock);
	if (impl->isInitialised.is_set())
	{
		if (impl->isInitialised.get())
		{
			return impl->isLicenseOk;
		}
		return false; // not initialised properly
	}
	return NP;
}

void Viveport::set_achievement(Name const& _achievementName)
{
	todo_implement;
}
