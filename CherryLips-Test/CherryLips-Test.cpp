// CherryLips-Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "../CherryLips/CherryLips.h"

#include <iostream>
#include <vector>

void testUploadObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest("my-bucket-name", "fv/Database2.accdb");
  std::string etag = client->UploadObject(&dest, "E:\\Database2.accdb");
}

void testUploadObjectMemory(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest("my-bucket-name", "fv/1.txt");
  const char* data = "123123123123123";
  std::string etag = client->UploadObjectMemory(&dest, data, strlen(data));
}

void testIsBucketExists(MinioClient* client) { 
    bool b = client->IsBucketExists("my-bucket-name");
}

void testComposeObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest("my-bucket-name", "fv/all-object");
  MinioClient::RemoteObjectStruct sources[2];
  sources[0].assign("my-bucket-name", "fv/Database2.accdb");
  sources[1].assign("my-bucket-name", "fv/my-object");
  client->ComposeObject(&dest, sources, 2);
}

void testCopyObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest("my-bucket-name", "fv/Database3.accdb");
  MinioClient::RemoteObjectStruct src("my-bucket-name", "fv/Database2.accdb");
  client->CopyObject(&dest, &src);
}

void testDownloadObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct src("my-bucket-name", "fv/Database2.accdb");
  client->DownloadObject(&src, "E:\\1.accdb");
}

bool ReadObjectCallback(const char* datachunk, size_t datalen,
    void* userData) {
  std::cout << datachunk;
  return true;
}
void testReadObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct src("my-bucket-name", "fv/my-object");
  client->ReadObject(&src, ReadObjectCallback);
}

void testGenerateObjectUrl(MinioClient* client) {
  MinioClient::RemoteObjectStruct src("my-bucket-name", "fv/my-object");
  std::string url = client->GenerateObjectUrl(&src, 30 * 60);
}

void ListBucketsCallback(const char* bucketName, void* userData) {
  std::cout << bucketName << std::endl;
}
void testListBuckets(MinioClient* client) {
  client->ListBuckets(ListBucketsCallback, NULL);
}

void testMakeBucket(MinioClient* client) { client->MakeBucket("mensong"); }

void testRemoveObject(MinioClient* client) {
  MinioClient::RemoteObjectStruct dest("my-bucket-name", "fv/Database3.accdb");
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

  client->SetBucketTags("my-bucket-name", &tags[0]);
  client->GetBucketTags("my-bucket-name", GetBucketTagsCallback);
  client->RemoveBucketTags("my-bucket-name");
  client->GetBucketTags("my-bucket-name", GetBucketTagsCallback);
}

void testSetObjectTags(MinioClient* client) {
  std::string tags;
  tags += "key1=val1";
  tags += '\0';
  tags += "key2=val2";
  tags += '\0';
  tags += '\0';

  MinioClient::RemoteObjectStruct src("my-bucket-name", "fv/my-object");
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

  testUploadObject(client);
  testUploadObjectMemory(client);
  testIsBucketExists(client);
  testComposeObject(client);
  testCopyObject(client);
  testDownloadObject(client);
  testReadObject(client);
  testGenerateObjectUrl(client);
  testListBuckets(client);
  testMakeBucket(client);
  testRemoveObject(client);
  testSetBucketTags(client);
  testSetObjectTags(client);

  CherryLips::Ins().FreeClient(&client);

  return 0;
}
