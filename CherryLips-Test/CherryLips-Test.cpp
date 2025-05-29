// CherryLips-Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "../CherryLips/CherryLips.h"

#include <iostream>
#include <vector>

#define bucket "test"
#define object "fv/my-object"


void testMakeBucket(MinioClient* client) { client->MakeBucket(bucket); }

void testUploadObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(bucket, object);
  std::string etag = client->UploadObject(&dest, "CherryLips-Test.cpp");
}

void testUploadObjectMemory(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(bucket, "fv/1.txt");
  const char* data = "123123123123123";
  std::string etag = client->UploadObjectMemory(&dest, data, strlen(data));
}

void testIsBucketExists(MinioClient* client) { 
    bool b = client->IsBucketExists(bucket);
}

void testComposeObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(bucket, "fv/all-object");
  MinioClient::RemoteObjectStruct sources[2];
  sources[0].assign(bucket, "fv/my-object1");
  sources[1].assign(bucket, "fv/my-object");
  client->ComposeObject(&dest, sources, 2);
}

void testCopyObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(bucket, "fv/my-object-copy");
  MinioClient::RemoteObjectStruct src(bucket, object);
  client->CopyObject(&dest, &src);
}

void testDownloadObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct src(bucket, object);
  client->DownloadObject(&src, "1.txt");
}

bool ReadObjectCallback(const char* datachunk, size_t datalen,
    void* userData) {
  std::cout << datachunk;
  return true;
}
void testReadObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct src(bucket, object);
  client->ReadObject(&src, ReadObjectCallback);
}

void testGenerateObjectUrl(MinioClient* client) {
  MinioClient::RemoteObjectStruct src(bucket, object);
  std::string url = client->GenerateObjectUrl(&src, 30 * 60);
}

void ListBucketsCallback(const char* bucketName, void* userData) {
  std::cout << bucketName << std::endl;
}
void testListBuckets(MinioClient* client) {
  client->ListBuckets(ListBucketsCallback, NULL);
}

void testRemoveObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest(bucket, "fv/my-object-copy");
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

  client->SetBucketTags(bucket, &tags[0]);
  client->GetBucketTags(bucket, GetBucketTagsCallback);
  client->RemoveBucketTags(bucket);
  client->GetBucketTags(bucket, GetBucketTagsCallback);
}

void testSetObjectTags(MinioClient* client) {
  std::string tags;
  tags += "key1=val1";
  tags += '\0';
  tags += "key2=val2";
  tags += '\0';
  tags += '\0';

  MinioClient::RemoteObjectStruct src(bucket, object);
  client->SetObjectTags(&src, &tags[0]);
  client->GetObjectTags(&src, GetBucketTagsCallback);
  client->RemoveObjectTags(&src);
  client->GetObjectTags(&src, GetBucketTagsCallback);
}

int main() {
  const char** pp[9][2];

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

  CherryLips::Ins().FreeClient(&client);

  getchar();

  return 0;
}
