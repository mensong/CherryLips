#include "pch.h"
#include "CherryLips.h"
#include <sstream>
#include <miniocpp/client.h>


class MinioClient_imp : public MinioClient {
  //
  minio::creds::Provider* m_provider = NULL;

  // Create S3 client.
  minio::s3::Client m_client;

  std::string m_buffer;
  std::string m_errorBuffer;

 public:
  MinioClient_imp(minio::s3::BaseUrl& base_url,
                  minio::creds::Provider* const provider = nullptr)
      : m_client(base_url, provider) {
    m_provider = provider;
  }

  virtual ~MinioClient_imp() {
    if (m_provider) {
      delete m_provider;
      m_provider = NULL;
    }
  }

  const char* GetLastError() override { return m_errorBuffer.c_str(); }

  virtual const char* UploadObject(const RemoteObjectStruct* remoteObject,
                                   const char* localFilePath,
                                   PFN_ProgressCallback cb = NULL,
                                   void* userData = NULL) override {
    m_errorBuffer.clear();
    if (!remoteObject || !localFilePath) return NULL;

    // Create upload object arguments.
    minio::s3::UploadObjectArgs args;
    args.bucket = remoteObject->bucket;
    args.object = remoteObject->objectPath;
    args.filename = localFilePath;

    if (cb) {
      args.progress_userdata = userData;
      args.progressfunc = [&](minio::http::ProgressFunctionArgs args) -> bool {
        return cb(args.download_total_bytes, args.downloaded_bytes,
                  args.download_speed, args.upload_total_bytes,
                  args.uploaded_bytes, args.upload_speed, args.userdata);
      };
    }

    minio::s3::UploadObjectResponse resp = m_client.UploadObject(args);

	// Handle response.
	if (resp) {
		m_buffer = resp.etag;
		return m_buffer.c_str();
	}
	else {
		m_errorBuffer = resp.Error().String();
		return "";
	}
  }

  virtual const char* UploadObjectMemory(const RemoteObjectStruct* remoteObject,
                                         const char* uploadData, size_t dataLen,
                                         size_t partSize = 0,
                                         PFN_ProgressCallback cb = NULL,
                                         void* userData = NULL) override {
    m_errorBuffer.clear();
    if (!remoteObject || !uploadData || dataLen == 0) return "";

    std::string sbuf(uploadData, dataLen);
    std::istringstream iss(sbuf, std::ios_base::binary);

    minio::s3::PutObjectArgs args(iss, dataLen, partSize);
    args.bucket = remoteObject->bucket;
    args.object = remoteObject->objectPath;

    if (cb) {
      args.progress_userdata = userData;
      args.progressfunc = [&](minio::http::ProgressFunctionArgs args) -> bool {
        return cb(args.download_total_bytes, args.downloaded_bytes,
                  args.download_speed, args.upload_total_bytes,
                  args.uploaded_bytes, args.upload_speed, args.userdata);
      };
    }

    // Call put object.
    minio::s3::PutObjectResponse resp = m_client.PutObject(args);

    // Handle response.
    if (resp) {
      m_buffer = resp.etag;
      return m_buffer.c_str();
    } else {
      m_errorBuffer = resp.Error().String();
      return "";
    }

  }

  bool IsBucketExists(const char* bucket) override {
    m_errorBuffer.clear();

    if (!bucket) {
      return false;
    }
    // Create bucket exists arguments.
    minio::s3::BucketExistsArgs args;
    args.bucket = bucket;

    // Call bucket exists.
    minio::s3::BucketExistsResponse resp = m_client.BucketExists(args);

    // Handle response.
    if (resp) {
      return resp.exist;
    } else {
      m_errorBuffer = resp.Error().String();
    }
    return false;
  }

  bool ComposeObject(const RemoteObjectStruct* dest,
                     const RemoteObjectStruct* arrSources,
                     int sourcesCount) override {
    m_errorBuffer.clear();
    if (!dest || !arrSources || sourcesCount < 2) return false;

    // Create compose object arguments.
    minio::s3::ComposeObjectArgs args;
    args.bucket = dest->bucket;
    args.object = dest->objectPath;

    std::list<minio::s3::ComposeSource> lstSources;
    for (int i = 0; i < sourcesCount; i++) {
      const RemoteObjectStruct& ros = arrSources[i];
      minio::s3::ComposeSource source;
      source.bucket = ros.bucket;
      source.object = ros.objectPath;
      lstSources.push_back(source);
    }

    args.sources = lstSources;

    // Call compose object.
    minio::s3::ComposeObjectResponse resp = m_client.ComposeObject(args);

    // Handle response.
    bool b = (bool)resp;
    if (resp) {
      return b;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }

  bool CopyObject(const RemoteObjectStruct* dest,
                  const RemoteObjectStruct* source) override {
    m_errorBuffer.clear();
    // Create copy object arguments.
    minio::s3::CopyObjectArgs args;
    args.bucket = dest->bucket;
    args.object = dest->objectPath;

    minio::s3::CopySource copysource;
    copysource.bucket = source->bucket;
    copysource.object = source->objectPath;
    args.source = copysource;

    // Call copy object.
    minio::s3::CopyObjectResponse resp = m_client.CopyObject(args);

    // Handle response.
    if (resp) {
      return true;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }

  bool DownloadObject(const RemoteObjectStruct* remoteObject,
                      const char* localFilePath) override {
    m_errorBuffer.clear();
    // Create download object arguments.
    minio::s3::DownloadObjectArgs args;
    args.bucket = remoteObject->bucket;
    args.object = remoteObject->objectPath;
    args.filename = localFilePath;

    // Call download object.
    minio::s3::DownloadObjectResponse resp = m_client.DownloadObject(args);

    // Handle response.
    if (resp) {
      return true;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }

  bool ReadObject(const RemoteObjectStruct* remoteObject,
                  PFN_ReadObjectCallback readCB,
                  PFN_ProgressCallback progressCB = NULL,
                  void* readUserData = 0, void* progressUserData = 0) override {
    m_errorBuffer.clear();
    if (!remoteObject || !readCB) return false;
    // Create get object arguments.
    minio::s3::GetObjectArgs args;
    args.bucket = remoteObject->bucket;
    args.object = remoteObject->objectPath;
    args.userdata = readUserData;
    args.datafunc = [&](minio::http::DataFunctionArgs args) -> bool {
      return readCB(args.datachunk.data(), args.datachunk.size(), args.userdata);
    };

    if (progressCB)
    {
      args.progress_userdata = progressUserData;
      args.progressfunc = [&](minio::http::ProgressFunctionArgs args) -> bool {
        return progressCB(args.download_total_bytes, args.downloaded_bytes,
                          args.download_speed, args.upload_total_bytes,
                          args.uploaded_bytes, args.upload_speed, args.userdata);
      };
    }

    // Call get object.
    minio::s3::GetObjectResponse resp = m_client.GetObject(args);

    // Handle response.
    if (resp) {
      return true;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }

  const char* GenerateObjectUrl(const RemoteObjectStruct* remoteObject,
                                unsigned int expirySeconds,
                                Method method = Method::kGet) override {
    m_errorBuffer.clear();
    if (!remoteObject) return "";

    // Create get presigned object url arguments.
    minio::s3::GetPresignedObjectUrlArgs args;
    args.bucket = remoteObject->bucket;
    args.object = remoteObject->objectPath;
    args.method = (minio::http::Method)method;
    args.expiry_seconds = expirySeconds;

    // Call get presigned object url.
    minio::s3::GetPresignedObjectUrlResponse resp =
        m_client.GetPresignedObjectUrl(args);

    // Handle response.
    if (resp) {
      m_buffer = resp.url;
      return m_buffer.c_str();
    } else {
      m_errorBuffer = resp.Error().String();
      return "";
    }
  }

  bool ListBuckets(PFN_ListBucketsCallback cb, void* userData = NULL) override {
    m_errorBuffer.clear();
    if (!cb) return false;

    // Call list buckets.
    minio::s3::ListBucketsResponse resp = m_client.ListBuckets();

    // Handle response.
    if (resp) {
      for (auto& bucket : resp.buckets) {
        cb(bucket.name.c_str(), userData);
        //bucket.creation_date.ToHttpHeaderValue()
      }
      return true;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }

  bool MakeBucket(const char* bucketName) override {
    m_errorBuffer.clear();
    if (!bucketName) return false;

    // Create make bucket arguments.
    minio::s3::MakeBucketArgs args;
    args.bucket = bucketName;

    // Call make bucket.
    minio::s3::MakeBucketResponse resp = m_client.MakeBucket(args);

    // Handle response.
    if (resp) {
      return true;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }

  bool RemoveBucket(const char* bucketName) override {
    m_errorBuffer.clear();
    if (!bucketName) return false;

      // Create remove bucket arguments.
    minio::s3::RemoveBucketArgs args;
    args.bucket = bucketName;

    // Call remove bucket.
    minio::s3::RemoveBucketResponse resp = m_client.RemoveBucket(args);

    // Handle response.
    if (resp) {
      return true;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }

  bool RemoveObject(const RemoteObjectStruct* remoteObject) override {
    m_errorBuffer.clear();
    if (!remoteObject) return false;

    // Create remove object arguments.
    minio::s3::RemoveObjectArgs args;
    args.bucket = remoteObject->bucket;
    args.object = remoteObject->objectPath;

    // Call remove object.
    minio::s3::RemoveObjectResponse resp = m_client.RemoveObject(args);

    // Handle response.
    if (resp) {
      return true;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }

  bool SetBucketTags(const char* bucketName,
                     const char* keyvalueListStr) override {
    m_errorBuffer.clear();
    if (!bucketName || !keyvalueListStr) return false;

    // Create set bucket tags arguments.
    minio::s3::SetBucketTagsArgs args;
    args.bucket = bucketName;
    const char* p = keyvalueListStr;
    while (p) {
      const char* str = p;
      if (!str) break;
      std::string s = str;
      if (s.empty()) break;

      size_t idx = s.find('=');
      if (idx != std::string::npos) {
        std::string key = s.substr(0, idx);
        std::string val = s.substr(idx + 1);
        args.tags[key] = val;
      }

      int curLen = strlen(str);
      p += (curLen + 1);
    }

    // Call set bucket tags.
    minio::s3::SetBucketTagsResponse resp = m_client.SetBucketTags(args);

    // Handle response.
    if (resp) {
      return true;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }


 bool SetObjectTags(const RemoteObjectStruct* remoteObject,
                     const char* keyvalueListStr) override {
    m_errorBuffer.clear();
   if (!remoteObject || !keyvalueListStr) return false;

   // Create set object tags arguments.
   minio::s3::SetObjectTagsArgs args;
   args.bucket = remoteObject->bucket;
   args.object = remoteObject->objectPath;

   const char* p = keyvalueListStr;
   while (p) {
     const char* str = p;
     if (!str) break;
     std::string s = str;
     if (s.empty()) break;

     size_t idx = s.find('=');
     if (idx != std::string::npos) {
       std::string key = s.substr(0, idx);
       std::string val = s.substr(idx + 1);
       args.tags[key] = val;
     }

     int curLen = strlen(str);
     p += (curLen + 1);
   }

   // Call set object tags.
   minio::s3::SetObjectTagsResponse resp = m_client.SetObjectTags(args);

   // Handle response.
   if (resp) {
     return true;
   } else {
     m_errorBuffer = resp.Error().String();
     return false;
   }
  }

 bool GetBucketTags(const char* bucketName, PFN_GetTagsCallback cb,
                     void* userData = NULL) override {
    m_errorBuffer.clear();
    if (!bucketName || !cb) return false;

    // Create get bucket tags arguments.
    minio::s3::GetBucketTagsArgs args;
    args.bucket = bucketName;

    // Call get bucket tags.
    minio::s3::GetBucketTagsResponse resp = m_client.GetBucketTags(args);

    // Handle response.
    if (resp) {
      for (auto& [key, value] : resp.tags) {
        cb(key.c_str(), value.c_str(), userData);
      }
      return true;
    } else {
      m_errorBuffer = resp.Error().String();
      return false;
    }
  }

 bool GetObjectTags(const RemoteObjectStruct* remoteObject,
                    PFN_GetTagsCallback cb, void* userData = NULL) override {
    m_errorBuffer.clear();
   if (!remoteObject || !cb) return false;

    // Create get object tags arguments.
   minio::s3::GetObjectTagsArgs args;
   args.bucket = remoteObject->bucket;
   args.object = remoteObject->objectPath;

   // Call get object tags.
   minio::s3::GetObjectTagsResponse resp = m_client.GetObjectTags(args);

   // Handle response.
   if (resp) {
     for (auto& [key, value] : resp.tags) {
       cb(key.c_str(), value.c_str(), userData);
     }
     return true;
   } else {
     m_errorBuffer = resp.Error().String();
     return false;
   }
 }


 bool RemoveBucketTags(const char* bucketName) override {
   m_errorBuffer.clear();
   if (!bucketName) return false;

   // Create delete bucket tags arguments.
   minio::s3::DeleteBucketTagsArgs args;
   args.bucket = bucketName;

   // Call delete bucket tags.
   minio::s3::DeleteBucketTagsResponse resp = m_client.DeleteBucketTags(args);

   // Handle response.
   if (resp) {
     return true;
   } else {
     m_errorBuffer = resp.Error().String();
     return false;
   }
 }

 bool RemoveObjectTags(const RemoteObjectStruct* remoteObject) override {
   m_errorBuffer.clear();
   if (!remoteObject) return false;

   // Create delete object tags arguments.
   minio::s3::DeleteObjectTagsArgs args;
   args.bucket = remoteObject->bucket;
   args.object = remoteObject->objectPath;

   // Call delete object tags.
   minio::s3::DeleteObjectTagsResponse resp = m_client.DeleteObjectTags(args);

   // Handle response.
   if (resp) {
     return true;
   } else {
     m_errorBuffer = resp.Error().String();
     return false;
   }
 }
};

CherryLips g_ins;
MINIO_API class CherryLips* MINIO_Instance() { return &g_ins; }

MINIO_API MinioClient* __cdecl NewClient(const char* url,
                                         const char* access_key,
                                         const char* secret_key,
                                         const char* session_token) {
  if (!url) return NULL;

  minio::http::Url uri = minio::http::Url::Parse(url);

  // Create S3 base URL.
  std::string host = uri.host;
  if (uri.port != 0) host += ':' + std::to_string(uri.port);
  minio::s3::BaseUrl base_url(host, uri.https);

  if (access_key && secret_key) {
    // Create credential provider.
    minio::creds::StaticProvider* provider = new minio::creds::StaticProvider(
        access_key, secret_key, (session_token ? session_token : ""));

    return new MinioClient_imp(base_url, provider);
  } else {
    return new MinioClient_imp(base_url, nullptr);
  }
}

MINIO_API void __cdecl FreeClient(MinioClient** ppClient) {
  delete *ppClient;
  *ppClient = NULL;
}
