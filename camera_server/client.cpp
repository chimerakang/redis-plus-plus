#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>

#include "image_helper/image_helper.h"
#include "cxxopts.hpp"
#include "image_helper/image_utils.h"
#include "MJPEGWriter.h"

///#define ENABLE_ALPR 1
#ifdef ENABLE_ALPR
#include "ultimateALPR-SDK-API-PUBLIC.h"

#if ULTALPR_SDK_OS_ANDROID 
#	define ASSET_MGR_PARAM() __sdk_android_assetmgr, 
#else
#	define ASSET_MGR_PARAM() 
#endif /* ULTALPR_SDK_OS_ANDROID */

using namespace ultimateAlprSdk;
#endif
using namespace sw::redis::image_helper;


bool VERBOSE = false;
bool STREAM_MODE = true;
std::string redisInputKey = "custom:image";
std::string redisInputCameraParametersKey = "default:camera:parameters";
std::string redisHost = "127.0.0.1";
std::string rtspURL = "";
int redisPort = 6379;
int webPort = 10501;

struct cameraParams {
    uint width;
    uint height;
    uint channels;
};

#ifdef ENABLE_ALPR
// Configuration for ANPR deep learning engine
static const char* __jsonConfig =
"{"
"\"debug_level\": \"info\","
"\"debug_write_input_image_enabled\": false,"
"\"debug_internal_data_path\": \".\","
""
"\"num_threads\": -1,"
"\"gpgpu_enabled\": true,"
""
"\"klass_vcr_gamma\": 1.5,"
""
"\"detect_roi\": [0, 0, 0, 0],"
"\"detect_minscore\": 0.1,"
""
"\"pyramidal_search_enabled\": true,"
"\"pyramidal_search_sensitivity\": 0.28,"
"\"pyramidal_search_minscore\": 0.3,"
"\"pyramidal_search_min_image_size_inpixels\": 800,"
""
"\"recogn_minscore\": 0.3,"
"\"recogn_score_type\": \"min\""
"";

/*
* Parallel callback function used for notification. Not mandatory.
* More info about parallel delivery: https://www.doubango.org/SDKs/anpr/docs/Parallel_versus_sequential_processing.html
*/
class MyUltAlprSdkParallelDeliveryCallback : public UltAlprSdkParallelDeliveryCallback {
public:
	MyUltAlprSdkParallelDeliveryCallback(const std::string& charset) : m_strCharset(charset) {}
	virtual void onNewResult(const UltAlprSdkResult* result) const override {
		static size_t numParallelDeliveryResults = 0;
		ULTALPR_SDK_ASSERT(result != nullptr);
		const std::string& json = result->json();
		ULTALPR_SDK_PRINT_INFO("MyUltAlprSdkParallelDeliveryCallback::onNewResult(%d, %s, %zu): %s",
			result->code(),
			result->phrase(),
			++numParallelDeliveryResults,
			!json.empty() ? json.c_str() : "{}"
		);
	}
private:
	std::string m_strCharset;
};


void initUltimateEngine() {
    	bool isParallelDeliveryEnabled = true; // Single image -> no need for parallel processing
	bool isRectificationEnabled = false;
	bool isOpenVinoEnabled = false;
	bool isKlassLPCI_Enabled = false;
	bool isKlassVCR_Enabled = false;
	bool isKlassVMMR_Enabled = false;
	std::string charset = "latin";
	std::string openvinoDevice = "GPU";
	std::string assetsFolder = "./assets";
    	std::string licenseTokenData = "ANI6+wXQBUVDUFFVdzBBR1VQRkMuBApERW4nS1FTRkgvKTEAGTwQeEtZREJUdkdVV3tGCWl3akZOXFg4G3Q2Ymk7cUFYWygGCBQqDgZVSUUzPW5LaUUxRlUpImJcRkFkMRscJyQ6RlhxRTxODAtKNTE3MGRlRDFdVGZqVDc1fScXNH5IX1AlCzkjKRdRNUVbXGYMLz0/JigQBg5gVmNiW3oxeUtxCFU6I1J5WwsyXiEyGi1SQz0+QxgpJzIqXyxXV35eaDBSeU59Sn4VPgEGP2N6LzoeVls=";


	// Update JSON config
	std::string jsonConfig = __jsonConfig;
	jsonConfig += std::string(",\"recogn_rectify_enabled\": ") + (isRectificationEnabled ? "true" : "false");
	jsonConfig += std::string(",\"openvino_enabled\": ") + (isOpenVinoEnabled ? "true" : "false");
	if (!openvinoDevice.empty()) {
		jsonConfig += std::string(",\"openvino_device\": \"") + openvinoDevice + std::string("\"");
	}
	jsonConfig += std::string(",\"klass_lpci_enabled\": ") + (isKlassLPCI_Enabled ? "true" : "false");
	jsonConfig += std::string(",\"klass_vcr_enabled\": ") + (isKlassVCR_Enabled ? "true" : "false");
	jsonConfig += std::string(",\"klass_vmmr_enabled\": ") + (isKlassVMMR_Enabled ? "true" : "false");
	if (!assetsFolder.empty())
    	{
        	jsonConfig += std::string(",\"assets_folder\": \"") + assetsFolder + std::string("\"");
    	}

	// if (!licenseTokenFile.empty()) {
	// 	jsonConfig += std::string(",\"license_token_file\": \"") + licenseTokenFile + std::string("\"");
	// }
	if (!licenseTokenData.empty()) {
	 	jsonConfig += std::string(",\"license_token_data\": \"") + licenseTokenData + std::string("\"");
	}
	
	jsonConfig += "}"; // end-of-config

    UltAlprSdkResult result = UltAlprSdkEngine::init(jsonConfig.c_str());

    MyUltAlprSdkParallelDeliveryCallback parallelDeliveryCallbackCallback(charset);
	ULTALPR_SDK_ASSERT((result = UltAlprSdkEngine::init(
		ASSET_MGR_PARAM()
		jsonConfig.c_str(),
		isParallelDeliveryEnabled ? &parallelDeliveryCallbackCallback : nullptr
	)).isOK());

    if(result.isOK()) {
        std::cout << "UltralprSdkEngine init success" << std::endl;
    }
}
#endif

static int parseCommandLine(cxxopts::Options options, int argc, char** argv)
{
    auto result = options.parse(argc, argv);
    if (result.count("h")) {
        std::cout << options.help({"", "Group"});
        return EXIT_FAILURE;
    }

    if (result.count("v")) {
        VERBOSE = true;
        std::cerr << "Verbose mode enabled." << std::endl;
    }

    if (result.count("i")) {
        rtspURL = result["i"].as<std::string>();
        if (VERBOSE) {
            std::cerr << "Input rtsp url was set to `" << rtspURL << "`." << std::endl;
        }
    }
    else {
        if (VERBOSE) {
            std::cerr << "No input rtsp was specified. Input rtsp was set to default (" << rtspURL << ")." << std::endl;
        }
    }

    if (result.count("p")) {
        webPort = result["p"].as<int>();
        if (VERBOSE) {
            std::cerr << "Web port set to " << webPort << "." << std::endl;
        }
    }
    else {
        if (VERBOSE) {
            std::cerr << "No web port specified. Web port was set to " << webPort << "." << std::endl;
        }
    }


    if (result.count("u")) {
        STREAM_MODE = false;
        if (VERBOSE) {
            std::cerr << "Unique mode enabled." << std::endl;
        }
    }

    if (result.count("redis-port")) {
        redisPort = result["redis-port"].as<int>();
        if (VERBOSE) {
            std::cerr << "Redis port set to " << redisPort << "." << std::endl;
        }
    }
    else {
        if (VERBOSE) {
            std::cerr << "No redis port specified. Redis port was set to " << redisPort << "." << std::endl;
        }
    }

    if (result.count("redis-host")) {
        redisHost = result["redis-host"].as<std::string>();
        if (VERBOSE) {
            std::cerr << "Redis host set to " << redisHost << "." << std::endl;
        }
    }
    else {
        if (VERBOSE) {
            std::cerr << "No redis host was specified. Redis host was set to " << redisHost << "." << std::endl;
        }
    }

    if (result.count("camera-parameters")) {
        redisInputCameraParametersKey = result["camera-parameters"].as<std::string>();
        if (VERBOSE) {
            std::cerr << "Camera parameters output key was set to " << redisInputCameraParametersKey << std::endl;
        }
    }
    else {
        if (VERBOSE) {
            std::cerr << "No camera parameters output key specified. Camera parameters output key was set to " << redisInputCameraParametersKey << std::endl;
        }
    }

    return 0;
}

void onImagePublished(redisAsyncContext* c, void* data, void* privdata)
{
    struct cameraParams* cameraParams = static_cast<struct cameraParams*>(privdata);
    if (cameraParams == NULL) {
        if(VERBOSE) {
            std::cerr << "Could not read camera parameters from private data." << std::endl;
        }
        return;
    }
    uint width = cameraParams->width;
    uint height = cameraParams->height;
    uint channels = cameraParams->channels;
    std::cout << "width:" << width << ",height:" << height << ",channels:" << channels << std::endl;


    redisReply *reply = (redisReply*) data;
    if  (reply == NULL)
    {
        return;
    }
    if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 3)
    {
        int length = reply->element[2]->len;

        //cv::Mat displayFrame;
        Image* cFrame = RedisImageHelper::dataToImage(reply->element[2]->str, width, height, channels, reply->element[2]->len);
        if (cFrame == NULL) {
            if (VERBOSE) {
                std::cerr << "Could not retrieve image from data." << std::endl;
            }
            return;
        }
        //cv::Mat frame = cv::Mat(cFrame->height(), cFrame->width(), CV_8UC3, (void*)cFrame->data());
        //cv::cvtColor(frame, displayFrame, CV_RGB2BGR);
	 std::cout << "width:" << cFrame->width() << ",height:" << cFrame->height() << ",length:" << length << std::endl;


#ifdef ENABLE_ALPR
        UltAlprSdkResult result;
        // Recognize/Process
        // We load the models when this function is called for the first time. This make the first inference slow.
        // Use benchmark application to compute the average inference time: https://github.com/DoubangoTelecom/ultimateALPR-SDK/tree/master/samples/c%2B%2B/benchmark

	ULTALPR_SDK_ASSERT((result = UltAlprSdkEngine::process(
            ULTALPR_SDK_IMAGE_TYPE_RGB24, // If you're using data from your camera then, the type would be YUV-family instead of RGB-family. https://www.doubango.org/SDKs/anpr/docs/cpp-api.html#_CPPv4N15ultimateAlprSdk22ULTALPR_SDK_IMAGE_TYPEE
            cFrame->data(),
            cFrame->width(),
            cFrame->height()
        )).isOK());
        ULTALPR_SDK_PRINT_INFO("Processing done.");

        // Print latest result
        if (result.json()) { // for parallel delivery the result will be printed by the callback function
            const std::string& json_ = result.json();
            if (!json_.empty()) {
                ULTALPR_SDK_PRINT_INFO("result: %s", json_.c_str());
            }
        }
	
#endif

        //cv::imshow("frame", displayFrame);
        //cv::waitKey(30);
        delete cFrame;
    }
}

int main(int argc, char** argv)
{

    cxxopts::Options options(argv[0], "Camera client sample program.");
    options.add_options()
            ("redis-host", "The host adress to which the redis client should try to connect", cxxopts::value<std::string>())
            ("redis-port", "The port to which the redis client should try to connect.", cxxopts::value<int>())
            ("i, input", "The redis input key where data are going to arrive.", cxxopts::value<std::string>())
            ("s, stream", "Activate stream mode. In stream mode the program will constantly process input data and publish output data. By default stream mode is enabled.")
            ("u, unique", "Activate unique mode. In unique mode the program will only read and output data one time.")
            ("v, verbose", "Enable verbose mode. This will print helpfull process informations on the standard error stream.")
            ("camera-parameters", "The redis input key where camera-parameters are going to arrive.", cxxopts::value<std::string>())
            ("p, port", "Open web service port.", cxxopts::value<int>())
            ("h, help", "Print this help message.");

    int retCode = parseCommandLine(options, argc, argv);
    if (retCode)
    {
        return EXIT_FAILURE;
    }

    RedisImageHelperSync clientSync(redisHost, redisPort, redisInputKey);
    if (!clientSync.connect()) {
        std::cerr << "Could not connect to redis server." << std::endl;
        return EXIT_FAILURE;
    }

    struct cameraParams contextData;
    contextData.width = clientSync.getInt(redisInputCameraParametersKey + ":width");
    contextData.height = clientSync.getInt(redisInputCameraParametersKey + ":height");
    contextData.channels = clientSync.getInt(redisInputCameraParametersKey + ":channels");



#ifdef ENABLE_ALPR
    initUltimateEngine();
#endif

    MJPEGWriter web(webPort); //Creates the MJPEGWriter class to stream on the given port
    // const std::string pipeline("rtspsrc location=rtsp://103.126.252.189:11011/stream0 latency=0 ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! appsink");

    ///cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
    VideoCapture cap("rtsp://103.126.252.189:11011/stream0");
    // VideoCapture cap(0);
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640); 
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    bool ok = cap.isOpened(); //Open rtsp
    if (!ok)
    {
        printf("no rtsp opened ;(.\n");
        pthread_exit(NULL);
    }
    Mat frame;
    cap >> frame;
    web.write(frame); //Writes a frame (Mat class from OpenCV) to the server
    frame.release();
    web.start(); //Starts the HTTP Server on the selected port
    while(cap.isOpened()){
        cap >> frame; 
        web.write(frame); 
        frame.release();
    }
    web.stop(); //Stops the HTTP Server


    if (STREAM_MODE) {
        RedisImageHelperAsync clientAsync(redisHost, redisPort, redisInputKey);
        if (!clientAsync.connect()) {
            std::cerr << "Could not connect to redis server." << std::endl;
            return EXIT_FAILURE;
        }
        clientAsync.subscribe(redisInputKey, onImagePublished, static_cast<void*>(&contextData));
        std::cout << "exit success" << std::endl;
        return EXIT_SUCCESS;
    }
    else {
        cv::Mat displayFrame;
        Image* image = clientSync.getImage(contextData.width, contextData.height, contextData.channels, redisInputKey);
        if (image == NULL) { std::cerr << "Error: Could not get camera frame, exiting..." << std::endl; return EXIT_FAILURE;}
        cv::Mat frame = cv::Mat(image->height(), image->width(), CV_8UC3, (void*)image->data());
        cv::cvtColor(frame, displayFrame, CV_RGB2BGR);
        cv::imshow("frame", displayFrame);
        cv::waitKey();
        delete image;

        return EXIT_SUCCESS;
    }

#ifdef ENABLE_ALPR
    UltAlprSdkResult result = UltAlprSdkEngine::deInit();
#endif
}

