#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>

int main()
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    Aws::Client::ClientConfiguration config;
    config.region = "us-east-1";  // Specify the AWS region

    Aws::S3::S3Client s3Client(config);

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket("my-bucket");
    request.SetKey("my-file.txt");
    request.SetBody("Hello, S3!");

    auto outcome = s3Client.PutObject(request);
    if (outcome.IsSuccess())
    {
        std::cout << "File uploaded successfully" << std::endl;
    }
    else
    {
        std::cout << "File upload failed: " << outcome.GetError().GetMessage() << std::endl;
    }

    Aws::ShutdownAPI(options);

    return 0;
}
