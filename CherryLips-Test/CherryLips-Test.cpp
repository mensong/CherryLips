// CherryLips-Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "../CherryLips/CherryLips.h"

#include <iostream>
#include <vector>

#define BUCKET_NAME "test"
#define OBJECT_PATH "fv/my-object"

bool _UploadProgressCallback(
	double download_total_bytes,
	double downloaded_bytes,
	double download_speed,
	double upload_total_bytes,
	double uploaded_bytes,
	double upload_speed, void* userdata)
{
	if (upload_total_bytes != 0) {
		double percent = uploaded_bytes / upload_total_bytes * 100;
		printf("\n%.2f%%", percent);
	}
	return true;
}

void testMakeBucket(MinioClient* client) { client->MakeBucket(BUCKET_NAME); }

void testUploadObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(BUCKET_NAME, OBJECT_PATH);
  std::string etag = client->UploadObject(&dest, "CherryLips-Test.cpp", _UploadProgressCallback, NULL);
  std::cout << etag << std::endl;
}

void testUploadObjectMemory(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(BUCKET_NAME, "fv/1.txt");
  const char* data = "123123123123123";
  std::string etag = client->UploadObjectMemory(&dest, data, strlen(data));
  std::cout << etag << std::endl;
}

void testIsBucketExists(MinioClient* client) { 
    bool b = client->IsBucketExists(BUCKET_NAME);
	std::cout << "bucket:" << BUCKET_NAME << (b ? "exists" : "not exists") << std::endl;
}

void testComposeObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(BUCKET_NAME, "fv/all-object");
  MinioClient::RemoteObjectStruct sources[2];
  sources[0].assign(BUCKET_NAME, "fv/my-object1");
  sources[1].assign(BUCKET_NAME, "fv/my-object");
  client->ComposeObject(&dest, sources, 2);
}

void testCopyObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(BUCKET_NAME, "fv/my-object-copy");
  MinioClient::RemoteObjectStruct src(BUCKET_NAME, OBJECT_PATH);
  client->CopyObject(&dest, &src);
}

void testDownloadObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct src(BUCKET_NAME, OBJECT_PATH);
  client->DownloadObject(&src, "1.txt");
}

bool ReadObjectCallback(const char* datachunk, size_t datalen,
    void* userData) {
  std::cout << datachunk;
  return true;
}
void testReadObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct src(BUCKET_NAME, OBJECT_PATH);
  client->ReadObject(&src, ReadObjectCallback);
}

void testGenerateObjectUrl(MinioClient* client) {
  MinioClient::RemoteObjectStruct src(BUCKET_NAME, OBJECT_PATH);
  std::string url = client->GenerateObjectUrl(&src, 30 * 60);
  std::cout << url << std::endl;
}

void ListBucketsCallback(const char* bucketName, void* userData) {
  std::cout << bucketName << std::endl;
}
void testListBuckets(MinioClient* client) {
  client->ListBuckets(ListBucketsCallback, NULL);
}

void testRemoveObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(BUCKET_NAME, "fv/my-object-copy");
  client->RemoveObject(&dest);
}

void GetBucketTagsCallback(const char* key, const char* value, void* userData) {
  std::cout << key << " = " << value << std::endl;
}

void testSetBucketTags(MinioClient* client) {
  std::string tags;
  tags += "key1=val1";
  tags += '\0';
  tags += "key2=val2";
  tags += '\0';
  tags += '\0';

  client->SetBucketTags(BUCKET_NAME, &tags[0]);
  client->GetBucketTags(BUCKET_NAME, GetBucketTagsCallback);
  client->RemoveBucketTags(BUCKET_NAME);
  client->GetBucketTags(BUCKET_NAME, GetBucketTagsCallback);
}

void testSetObjectTags(MinioClient* client) {
  std::string tags;
  tags += "key1=val1";
  tags += '\0';
  tags += "key2=val2";
  tags += '\0';
  tags += '\0';

  MinioClient::RemoteObjectStruct src(BUCKET_NAME, OBJECT_PATH);
  client->SetObjectTags(&src, &tags[0]);
  client->GetObjectTags(&src, GetBucketTagsCallback);
  client->RemoveObjectTags(&src);
  client->GetObjectTags(&src, GetBucketTagsCallback);
}

void testListObjects(MinioClient* client) {
	std::string arrJson = client->ListObjects(BUCKET_NAME, "fv/");
	std::cout << "testListObjects:" << std::endl
		      << arrJson << std::endl;
}

int main() {

  MinioClient* client = CherryLips::Ins().NewClient(
      "https://play.min.io", "Q3AM3UQ867SPQQA43P2F",
      "zuf+tfteSlswRu7BJ86wekitnifILbZam1KYY3TG", NULL);

  testMakeBucket(client);
  testUploadObject(client);
  testUploadObjectMemory(client);
  testIsBucketExists(client);
  testComposeObject(client);
  testCopyObject(client);
  testDownloadObject(client);
  testReadObject(client);
  testGenerateObjectUrl(client);
  testListBuckets(client);
  testRemoveObject(client);
  testSetBucketTags(client);
  testSetObjectTags(client);
  testListObjects(client);

  CherryLips::Ins().FreeClient(&client);

  getchar();

  return 0;
}
