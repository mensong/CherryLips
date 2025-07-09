#pragma once
#ifndef _AFX
#include <windows.h>
#endif
#include <stdint.h>

#include <string>

#ifdef CHERRYLIPS_EXPORTS
#define MINIO_API extern "C" __declspec(dllexport)
#else
#define MINIO_API extern "C" __declspec(dllimport)
#endif

class _MINIO_mutex {
public:
	_MINIO_mutex() { m_mutex = CreateMutex(NULL, FALSE, NULL); }

	bool lock() {
		if (m_mutex == INVALID_HANDLE_VALUE || !m_mutex) return false;
		return (WaitForSingleObject(m_mutex, INFINITE) == WAIT_OBJECT_0);
	}

	bool unlock() {
		if (m_mutex == INVALID_HANDLE_VALUE || !m_mutex) return false;
		return ReleaseMutex(m_mutex);
	}

private:
	HANDLE m_mutex;
};
__declspec(selectany) _MINIO_mutex g_MINIO_insMutex;

class MinioClient {
public:
	class RemoteObjectStruct {
	public:
		char* bucket;
		char* objectPath;

		RemoteObjectStruct() {
			bucket = NULL;
			objectPath = NULL;
		}

		RemoteObjectStruct(const char* _bucket, const char* _objectPath) {
			bucket = NULL;
			objectPath = NULL;
			assign(_bucket, _objectPath);
		}

		bool assign(const char* _bucket, const char* _objectPath) {
			release();

			if (_bucket) {
				size_t len = strlen(_bucket);
				if (len != 0) {
					bucket = new char[len + 1];
					strcpy_s(bucket, len + 1, _bucket);
				}
				else {
					bucket = NULL;
					return false;
				}
			}
			else {
				bucket = NULL;
				return false;
			}

			if (_objectPath) {
				size_t len = strlen(_objectPath);
				if (len != 0) {
					objectPath = new char[len + 1];
					strcpy_s(objectPath, len + 1, _objectPath);
				}
				else {
					objectPath = NULL;
					return false;
				}
			}
			else {
				objectPath = NULL;
				return false;
			}

			return true;
		}

		~RemoteObjectStruct() { release(); }

		void release() {
			if (bucket) {
				delete[] bucket;
				bucket = NULL;
			}
			if (objectPath) {
				delete[] objectPath;
				objectPath = NULL;
			}
		}
	};

	enum Method { kGet, kHead, kPost, kPut, kDelete };

	typedef bool (*PFN_ReadObjectCallback)(
		const char* datachunk, size_t datalen,
		void* userData);
	typedef bool (*PFN_ProgressCallback)(
		double download_total_bytes,
		double downloaded_bytes,
		double download_speed,
		double upload_total_bytes,
		double uploaded_bytes,
		double upload_speed, void* userdata);
	typedef void (*PFN_GetTagsCallback)(
		const char* key, const char* value,
		void* userData);
	typedef void (*PFN_ListBucketsCallback)(
		const char* bucketName,
		void* userData);

public:
	virtual const char* GetLastError() = 0;

	virtual const char* UploadObject(
		const RemoteObjectStruct* remoteObject,
		const char* localFilePath,
		size_t partSize = 0,
		PFN_ProgressCallback progressCB = NULL,
		void* progressUserData = NULL,
		DWORD timeoutMS = 0) = 0;

	virtual const char* UploadObjectMemory(
		const RemoteObjectStruct* remoteObject,
		const char* uploadData, size_t dataLen,
		size_t partSize = 0,
		PFN_ProgressCallback progressCB = NULL,
		void* progressUserData = NULL,
		DWORD timeoutMS = 0) = 0;

	virtual bool IsBucketExists(
		const char* bucket,
		DWORD timeoutMS = 0) = 0;

	virtual bool ComposeObject(
		const RemoteObjectStruct* dest,
		const RemoteObjectStruct* arrSources,
		int sourcesCount,
		DWORD timeoutMS = 0) = 0;

	virtual bool CopyObject(
		const RemoteObjectStruct* dest,
		const RemoteObjectStruct* source,
		DWORD timeoutMS = 0) = 0;

	virtual bool DownloadObject(
		const RemoteObjectStruct* remoteObject,
		const char* localFilePath,
		const char* version_id = NULL,
		PFN_ProgressCallback progressCB = NULL,
		void* progressUserData = NULL,
		DWORD timeoutMS = 0) = 0;

	virtual bool ReadObject(
		const RemoteObjectStruct* remoteObject,
		PFN_ReadObjectCallback readCB,
		PFN_ProgressCallback progressCB = NULL,
		void* readUserData = NULL,
		void* progressUserData = NULL,
		const char* version_id = NULL,
		DWORD timeoutMS = 0) = 0;

	virtual const char* GenerateObjectUrl(
		const RemoteObjectStruct* remoteObject,
		unsigned int expirySeconds,
		Method method = Method::kGet,
		const char* version_id = NULL,
		DWORD timeoutMS = 0) = 0;

	virtual bool ListBuckets(
		PFN_ListBucketsCallback cb, 
		void* userData = NULL,
		DWORD timeoutMS = 0) = 0;

	virtual const char* ListObjects(
		const char* bucket,
		const char* objectPathPrefix,
		bool recursive = false,
		bool include_versions = false,
		bool fetch_owner = false,
		bool include_user_metadata = false,
		DWORD timeoutMS = 0) = 0;

	virtual bool MakeBucket(
		const char* bucketName,
		DWORD timeoutMS = 0) = 0;

	virtual bool RemoveBucket(
		const char* bucketName,
		DWORD timeoutMS = 0) = 0;

	virtual bool RemoveObject(
		const RemoteObjectStruct* remoteObject,
		const char* version_id = NULL,
		DWORD timeoutMS = 0) = 0;

	// keyvalueListStr - e.g:"key1=val1\0key2=val2\0key3=val3\0\0";
	virtual bool SetBucketTags(
		const char* bucketName,
		const char* keyvalueListStr,
		DWORD timeoutMS = 0) = 0;

	// keyvalueListStr - e.g:"key1=val1\0key2=val2\0key3=val3\0\0";
	virtual bool SetObjectTags(
		const RemoteObjectStruct* remoteObject,
		const char* keyvalueListStr,
		const char* version_id = NULL,
		DWORD timeoutMS = 0) = 0;

	virtual bool GetBucketTags(
		const char* bucketName,
		PFN_GetTagsCallback cb, 
		void* userData = NULL,
		DWORD timeoutMS = 0) = 0;

	virtual bool GetObjectTags(
		const RemoteObjectStruct* remoteObject,
		PFN_GetTagsCallback cb, 
		void* userData = NULL,
		const char* version_id = NULL,
		DWORD timeoutMS = 0) = 0;

	virtual bool RemoveBucketTags(
		const char* bucketName,
		DWORD timeoutMS = 0) = 0;
	virtual bool RemoveObjectTags(
		const RemoteObjectStruct* remoteObject,
		const char* version_id = NULL,
		DWORD timeoutMS = 0) = 0;
};

MINIO_API class CherryLips* MINIO_Instance();

// 创建客户端
//  url - minio服务器地址，如:https://play.min.io
//  access_key - 如：Q3AM3UQ867SPQQA43P2F
//  secret_key - 如:zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG
//  session_token - 为空即可
MINIO_API MinioClient* __cdecl NewClient(
	const char* url,
	const char* access_key,
	const char* secret_key,
	const char* session_token);

// 销毁客户端
MINIO_API void __cdecl FreeClient(MinioClient** ppClient);

class CherryLips {
#define DEF_PROC(name) decltype(::name)* name

#define SET_PROC(ins, hDll, name) \
  ins->name = (decltype(::name)*)::GetProcAddress(hDll, #name)

public:
	CherryLips() {}

	inline static CherryLips& Ins() {
		g_MINIO_insMutex.lock();

		static CherryLips* s_ins = NULL;
		if (!s_ins) {
			HMODULE hDll = LoadLibraryFromCurrentDir("CherryLips.dll");
			decltype(MINIO_Instance)* funcIns =
				(decltype(MINIO_Instance)*)::GetProcAddress(hDll, "MINIO_Instance");
			if (funcIns) {
				s_ins = funcIns();
			}
			else {
				static CherryLips _ins;
				s_ins = &_ins;
			}

			SetFunctions(s_ins, hDll);
			s_ins->hDll = hDll;
		}

		g_MINIO_insMutex.unlock();

		return *s_ins;
	}

	inline HMODULE _HMODULE() { return hDll; }

	static void SetFunctions(CherryLips* ins, HMODULE hDll) {
		SET_PROC(ins, hDll, NewClient);
		SET_PROC(ins, hDll, FreeClient);
	}

	DEF_PROC(NewClient);
	DEF_PROC(FreeClient);

public:
	static HMODULE LoadLibraryFromCurrentDir(const char* dllName) {
		char selfPath[MAX_PATH];
		MEMORY_BASIC_INFORMATION mbi;
		HMODULE hModule =
			((::VirtualQuery(LoadLibraryFromCurrentDir, &mbi, sizeof(mbi)) != 0)
				? (HMODULE)mbi.AllocationBase
				: NULL);
		::GetModuleFileNameA(hModule, selfPath, MAX_PATH);
		std::string moduleDir(selfPath);
		size_t idx = moduleDir.find_last_of('\\');
		moduleDir = moduleDir.substr(0, idx);
		std::string modulePath = moduleDir + "\\" + dllName;
		char curDir[MAX_PATH];
		::GetCurrentDirectoryA(MAX_PATH, curDir);
		::SetCurrentDirectoryA(moduleDir.c_str());
		HMODULE hDll = LoadLibraryA(modulePath.c_str());
		::SetCurrentDirectoryA(curDir);
		if (!hDll) {
			DWORD err = ::GetLastError();
			char buf[10];
			sprintf_s(buf, "%u", err);
			::MessageBoxA(NULL, ("找不到" + modulePath + "模块:" + buf).c_str(),
				"找不到模块", MB_OK | MB_ICONERROR);
		}
		return hDll;
	}
	~CherryLips() {
		if (hDll) {
			FreeLibrary(hDll);
			hDll = NULL;
		}
	}

private:
	HMODULE hDll;

#undef DEF_PROC
#undef SET_PROC
};
