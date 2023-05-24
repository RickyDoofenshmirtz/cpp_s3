#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

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
void handleFileUpload(int clientSocket, const std::string& accessKeyId, const std::string& secretAccessKey, const std::string& bucketName, const std::string& keyName)
{
    // Receive the file data from the client
    std::vector<char> buffer(4096);
    std::stringstream fileStream;
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer.data(), buffer.size(), 0)) > 0)
    {
        fileStream.write(buffer.data(), bytesRead);
    }

    // Create a temporary file to store the uploaded data
    std::string tempFilePath = "/tmp/uploaded_file";
    std::ofstream tempFile(tempFilePath, std::ios::binary);
    tempFile << fileStream.rdbuf();
    tempFile.close();

    // Upload the file to AWS S3
    uploadToS3(accessKeyId, secretAccessKey, bucketName, keyName, tempFilePath);

    // Clean up the temporary file
    std::remove(tempFilePath.c_str());

    // Close the client socket
    close(clientSocket);
}

// Start the server and handle incoming connections
void startServer(int port, const std::string& accessKeyId, const std::string& secretAccessKey, const std::string& bucketName, const std::string& keyName)
{
    // Create a socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    // Set up the server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    // Bind the socket to the server address
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == -1)
    {
        std::cerr << "Failed to bind socket to address" << std::endl;
        close(serverSocket);
        return;
    }

    // Listen for incoming connections
    if (listen(serverSocket, 10) == -1)
    {
        std::cerr << "Failed to listen for connections" << std::endl;
        close(serverSocket);
        return;
    }

    std::cout << "Server started on port " << port << std::endl;

    // Accept incoming connections and handle them
    while (true)
    {
        sockaddr_in clientAddress{};
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);
        if (clientSocket == -1)
        {
            std::cerr << "Failed to accept incoming connection" << std::endl;
            continue;
        }

        std::cout << "Received a new connection" << std::endl;

        // Handle the file upload request in a separate thread
        std::thread fileUploadThread(handleFileUpload, clientSocket, accessKeyId, secretAccessKey, bucketName, keyName);
        fileUploadThread.detach();
    }

    // Close the server socket
    close(serverSocket);
}

int main()
{
    // Initialize AWS SDK
    initAWS();

    // AWS S3 credentials and bucket details
    std::string accessKeyId = "YOUR_ACCESS_KEY_ID";
    std::string secretAccessKey = "YOUR_SECRET_ACCESS_KEY";
    std::string bucketName = "YOUR_BUCKET_NAME";
    std::string keyName = "uploaded_file.txt";

    // Start the server on port 8080
    int port = 8080;
    startServer(port, accessKeyId, secretAccessKey, bucketName, keyName);

    // Shutdown AWS SDK
    shutdownAWS();

    return 0;
}
