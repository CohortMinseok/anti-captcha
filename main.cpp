#include <iostream>
#include <string>
#include <curl/curl.h>
#include <json/json.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <iterator>
#include <openssl/bio.h>
#include <openssl/evp.h>

// Function to read the response from the server
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to convert a .png file to a base64 string
std::string pngToBase64(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, buffer.data(), buffer.size());
    BIO_flush(bio);

    char* base64Data;
    long base64Length = BIO_get_mem_data(bio, &base64Data);

    std::string base64Encoded(base64Data, base64Length);
    BIO_free_all(bio);

    return base64Encoded;
}

// Function to send the request to Anti-Captcha API
std::string sendCaptchaToAntiCaptcha(const std::string& apiKey, const std::string& base64Image, 
                                     bool phrase = false, bool caseSensitive = false, int numeric = 0,
                                     bool math = false, int minLength = 0, int maxLength = 0, const std::string& comment = "") {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        // API endpoint for Anti-Captcha
        std::string url = "https://api.anti-captcha.com/createTask";

        // Create the JSON request body
        Json::Value jsonData;
        jsonData["clientKey"] = apiKey;
        Json::Value task;
        task["type"] = "ImageToTextTask";
        task["body"] = base64Image;

        // Optional parameters
        if (phrase) task["phrase"] = 1;
        if (caseSensitive) task["case"] = 1;
        if (numeric > 0) task["numeric"] = numeric;
        if (math) task["math"] = 1;
        if (minLength > 0) task["minLength"] = minLength;
        if (maxLength > 0) task["maxLength"] = maxLength;
        if (!comment.empty()) task["comment"] = comment;

        jsonData["task"] = task;

        // Convert JSON to string
        Json::StreamWriterBuilder writer;
        std::string requestBody = Json::writeString(writer, jsonData);

        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());

        // Set headers
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set up the write callback to capture the response
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    curl_global_cleanup();
    return readBuffer;
}

// Main function
int main() {
    std::string apiKey = "your-anti-captcha-api-key";  // Your API key
    std::string imagePath = "your-captcha-image.png";  // The CAPTCHA image file path

    // Convert PNG to base64
    std::string base64Image = pngToBase64(imagePath);

    // Send the request to Anti-Captcha with all parameters
    std::string response = sendCaptchaToAntiCaptcha(apiKey, base64Image, true, true, 1, true, 5, 10, "Solve this math CAPTCHA");

    // Print the response from the API
    std::cout << "Response from Anti-Captcha: " << response << std::endl;

    return 0;
}
