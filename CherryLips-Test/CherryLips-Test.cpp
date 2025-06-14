// CherryLips-Test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "../CherryLips/CherryLips.h"

#include <iostream>
#include <vector>

#define BUCKET_NAME "test"
#define OBJECT_PATH "fv/my-object"
#define TIMEOUT 10000

void printError(MinioClient* client)
{
	std::string err = client->GetLastError();
	if (!err.empty())
	{
		std::cout << "Error: " << err << std::endl;
	}

	std::cout << std::endl;
}

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

void testMakeBucket(MinioClient* client) { 
	std::cout << __FUNCTION__ << std::endl;
	client->MakeBucket(BUCKET_NAME, TIMEOUT); 
	printError(client);
}

void testUploadObject(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	MinioClient::RemoteObjectStruct dest(BUCKET_NAME, OBJECT_PATH);
	std::string etag = client->UploadObject(&dest, "CherryLips-Test.cpp", _UploadProgressCallback, NULL, TIMEOUT);
	std::cout << etag << std::endl;
	printError(client);
}

void testUploadObjectMemory(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	MinioClient::RemoteObjectStruct dest(BUCKET_NAME, "fv/1.txt");
	const char* data = "123123123123123";
	std::string etag = client->UploadObjectMemory(&dest, data, strlen(data), 0, NULL, NULL, TIMEOUT);
	std::cout << etag << std::endl;
	printError(client);
}

void testIsBucketExists(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	bool b = client->IsBucketExists(BUCKET_NAME, TIMEOUT);
	std::cout << "bucket:" << BUCKET_NAME << (b ? " exists" : " not exists") << std::endl;
	printError(client);
}

void testComposeObject(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	MinioClient::RemoteObjectStruct dest(BUCKET_NAME, "fv/all-object");
	MinioClient::RemoteObjectStruct sources[2];
	sources[0].assign(BUCKET_NAME, "fv/my-object1");
	sources[1].assign(BUCKET_NAME, "fv/my-object");
	client->ComposeObject(&dest, sources, 2, TIMEOUT);
	printError(client);
}

void testCopyObject(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	MinioClient::RemoteObjectStruct dest(BUCKET_NAME, "fv/my-object-copy");
	MinioClient::RemoteObjectStruct src(BUCKET_NAME, OBJECT_PATH);
	client->CopyObject(&dest, &src, TIMEOUT);
	printError(client);
}

void testDownloadObject(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	MinioClient::RemoteObjectStruct src(BUCKET_NAME, OBJECT_PATH);
	client->DownloadObject(&src, "1.txt", NULL, NULL, NULL, TIMEOUT);
	printError(client);
}

bool ReadObjectCallback(const char* datachunk, size_t datalen,
	void* userData) {
	std::cout << datachunk;
	return true;
}
void testReadObject(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	MinioClient::RemoteObjectStruct src(BUCKET_NAME, "fv/1.txt");
	client->ReadObject(&src, ReadObjectCallback, NULL, NULL, NULL, NULL, TIMEOUT);
	printError(client);
}

void testGenerateObjectUrl(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	MinioClient::RemoteObjectStruct src(BUCKET_NAME, OBJECT_PATH);
	std::string url = client->GenerateObjectUrl(&src, 30 * 60, MinioClient::kGet, NULL, TIMEOUT);
	std::cout << url << std::endl;
	printError(client);
}

void ListBucketsCallback(const char* bucketName, void* userData) {
	std::cout << bucketName << std::endl;
}
void testListBuckets(MinioClient* client) {
	client->ListBuckets(ListBucketsCallback, NULL, TIMEOUT);
	printError(client);
}

void testRemoveObject(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	MinioClient::RemoteObjectStruct dest(BUCKET_NAME, "fv/my-object-copy");
	client->RemoveObject(&dest, NULL, TIMEOUT);
	printError(client);
}

void GetBucketTagsCallback(const char* key, const char* value, void* userData) {
	std::cout << key << " = " << value << std::endl;
}

void testSetBucketTags(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	std::string tags;
	tags += "key1=val1";
	tags += '\0';
	tags += "key2=val2";
	tags += '\0';
	tags += '\0';

	client->SetBucketTags(BUCKET_NAME, &tags[0], TIMEOUT);
	printError(client);
	client->GetBucketTags(BUCKET_NAME, GetBucketTagsCallback, NULL, TIMEOUT);
	printError(client);
	client->RemoveBucketTags(BUCKET_NAME, TIMEOUT);
	printError(client);
	client->GetBucketTags(BUCKET_NAME, GetBucketTagsCallback, NULL, TIMEOUT);
	printError(client);
}

void testSetObjectTags(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	std::string tags;
	tags += "key1=val1";
	tags += '\0';
	tags += "key2=val2";
	tags += '\0';
	tags += '\0';

	MinioClient::RemoteObjectStruct src(BUCKET_NAME, OBJECT_PATH);
	client->SetObjectTags(&src, &tags[0], NULL, TIMEOUT);
	printError(client);
	client->GetObjectTags(&src, GetBucketTagsCallback, NULL, NULL, TIMEOUT);
	printError(client);
	client->RemoveObjectTags(&src, NULL, TIMEOUT);
	printError(client);
	client->GetObjectTags(&src, GetBucketTagsCallback, NULL, NULL, TIMEOUT);
	printError(client);
}

void testListObjects(MinioClient* client) {
	std::cout << __FUNCTION__ << std::endl;
	std::string arrJson = client->ListObjects(BUCKET_NAME, "fv/", false, false, false, false, TIMEOUT);
	std::cout << arrJson << std::endl;
	printError(client);
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

	return 0;
}
