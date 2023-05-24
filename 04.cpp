#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

using boost::asio::ip::tcp;

// AWS SDK initialization
void initAWS()
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
}

// AWS SDK shutdown
void shutdownAWS()
{
    Aws::ShutdownAPI(options);
}

// Uploads a file to AWS S3
bool uploadToS3(const std::string& accessKeyId, const std::string& secretAccessKey, const std::string& bucketName, const std::string& keyName, const std::string& filePath)
{
    // Set up AWS credentials and S3 client
    Aws::Auth::AWSCredentials credentials(accessKeyId, secretAccessKey);
    Aws::S3::S3Client s3Client(credentials);

    // Set up the PutObjectRequest
    Aws::S3::Model::PutObjectRequest objectRequest;
    objectRequest.SetBucket(bucketName);
    objectRequest.SetKey(keyName);

    // Read the file data
    std::ifstream fileStream(filePath, std::ios::binary | std::ios::ate);
    if (!fileStream)
    {
        std::cout << "Failed to open file: " << filePath << std::endl;
        return false;
    }
    std::streamsize fileSize = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);
    std::vector<unsigned char> fileData(fileSize);
    if (!fileStream.read(reinterpret_cast<char*>(fileData.data()), fileSize))
    {
        std::cout << "Failed to read file: " << filePath << std::endl;
        return false;
    }
    fileStream.close();

    // Set the file data in the PutObjectRequest
    auto objectData = Aws::MakeShared<Aws::StringStream>("ObjectName");
    objectData->write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
    objectRequest.SetBody(objectData);

    // Upload the file to S3
    auto putObjectOutcome = s3Client.PutObject(objectRequest);
    if (!putObjectOutcome.IsSuccess())
    {
        std::cout << "Failed to upload file to S3: " << putObjectOutcome.GetError().GetMessage() << std::endl;
        return false;
    }

    std::cout << "File uploaded to S3 successfully!" << std::endl;
    return true;
}

// Handle the file upload request
void handleFileUpload(tcp::socket& socket, const std::string& accessKeyId, const std::string& secretAccessKey, const std::string& bucketName, const std::string& keyName)
{
    // Read the file data from the socket
    boost::asio::streambuf buffer;
    boost::system::error_code error;
    size_t bytesRead = boost::asio::read(socket, buffer, boost::asio::transfer_all(), error);
    if (error && error != boost::asio::error::eof)
    {
        std::cout << "Failed to read file data: " << error.message() << std::endl;
        return;
    }

    // Create a temporary file to store the uploaded data
    std::string tempFilePath = "/tmp/uploaded_file";
    std::ofstream tempFile(tempFilePath, std::ios::binary);
    std::ostream tempFileStream(&tempFile);
    tempFileStream << &buffer;
    tempFile.close();

    // Upload the file to AWS S3
    uploadToS3(accessKeyId, secretAccessKey, bucketName, keyName, tempFilePath);

    // Clean up the temporary file
    std::remove(tempFilePath.c_str());
}

// Start the server and handle incoming connections
void startServer(int port, const std::string& accessKeyId, const std::string& secretAccessKey, const std::string& bucketName, const std::string& keyName)
{
    boost::asio::io_context ioContext;

    tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), port));

    std::cout << "Server started on port " << port << std::endl;

    while (true)
    {
        tcp::socket socket(ioContext);
        acceptor.accept(socket);

        std::cout << "Received a new connection" << std::endl;

        // Handle the file upload request
        handleFileUpload(socket, accessKeyId, secretAccessKey, bucketName, keyName);
    }
}

int main()
{
    // Initialize AWS SDK
    initAWS();

    // AWS S3 credentials and bucket details
    std::string accessKeyId = "ACCESS_KEY_ID";
    std::string secretAccessKey = "SECRET_ACCESS_KEY";
    std::string bucketName = "BUCKET_NAME";
    std::string keyName = "uploaded_file.txt";

    // Start the server on port 8080
    int port = 8080;
    startServer(port, accessKeyId, secretAccessKey, bucketName, keyName);

    // Shutdown AWS SDK
    shutdownAWS();

    return 0;
}


/*
    Error handling:
        The code provided does basic error checking and logging, but you may want to implement more robust error handling and logging mechanisms to handle various failure scenarios.

    Security:
        The code does not implement any authentication or authorization mechanisms.
        You might consider adding security measures such as user authentication or SSL/TLS encryption to protect the file upload process.

    Threading:
        The code handles each incoming connection in a separate thread, but it lacks proper thread management and may create too many threads in a high-traffic scenario.
        Consider implementing a thread pool or using an asynchronous approach to handle multiple connections more efficiently.

    Validation:
        It's important to perform validation on the received file data, such as checking the file size or file type, to ensure the integrity and security of the uploaded files.

    Performance optimizations:
        Depending on your specific use case, you may need to optimize the code for performance,
        such as handling large file uploads more efficiently or implementing streaming uploads directly to AWS S3 instead of storing files temporarily.

    Graceful shutdown:
        Implementing a graceful shutdown mechanism is essential to handle server termination or restart scenarios.
        This includes properly closing connections, saving the upload progress, and releasing any allocated resources.
*/