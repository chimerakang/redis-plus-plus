#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include "cxxopts.hpp"
#include "image_helper/image_utils.h"
#include "image_helper/image_helper.h"
#include <fstream>
#include <thread>
#include "ultimateALPR-SDK-API-PUBLIC.h"
#include "json.hpp"
#include "Config.hpp"
#include "MJPEGWriter.h"
#include "concurrentqueue.h"
#include "SSocket.h"
#include "macaddress.h"
#include <stdarg.h>  // For va_start, etc.
#include <memory>    // For std::unique_ptr
#include <unistd.h>
#include <sys/stat.h>
#define NO_STD_OPTIONAL
#include "mysql+++/mysql+++.h"

#include "yazi-mysql/mysql/Database.h"
#include "yazi-mysql/mysql/Table.h"
#include "yazi-mysql/mysql/Row.h"
using namespace yazi::mysql;

// #define ENABLE_ALPR 1
#ifdef ENABLE_ALPR

#if ULTALPR_SDK_OS_ANDROID
#define ASSET_MGR_PARAM() __sdk_android_assetmgr,
#else
#define ASSET_MGR_PARAM()
#endif /* ULTALPR_SDK_OS_ANDROID */

using namespace ultimateAlprSdk;
#endif
using namespace sw::redis::image_helper;
using json = nlohmann::json;
using namespace moodycamel;
using namespace RickyCorte;
using namespace daotk::mysql;

bool VERBOSE = false;
bool STREAM_MODE = true;
std::string redisInputKey = "custom:image";
std::string redisInputCameraParametersKey = "default:camera:parameters";
#ifdef ENABLE_ALPR
std::string redisHost = "localhost";
#else
std::string redisHost = "bomding.com";
#endif
ConfigFile *cfg = NULL;
connection DBConnection;

int redisPort = 6018;
int webPort1 = 10501;
int webPort2 = 10502;

struct PlateInfo {
    cv::Mat frame;
    std::string plate;
    float plate_confidence0, plate_confidence1;
    int px1, px2, py1, py2;
    float car_confidence;
    int cx1, cx2, cy1, cy2;
};

struct cameraParams {
    uint width;
    uint height;
    uint channels;
};

struct parking_device {
    string dev_id;
    string mac_address;
    bool online_status;
    string ipaddr;
    string device_key;
    string token;

    bool operator==(parking_device const& b) const noexcept
    {
        return this->dev_id == b.dev_id;
    }
};

ConcurrentQueue<cv::Mat> frameQueue;
ConcurrentQueue<PlateInfo> plateQueue;
ConcurrentQueue<string> uploadQueue;
/*
void check_error(AMY_SYSTEM_NS::error_code const& ec) {
    if (ec) {
        throw AMY_SYSTEM_NS::system_error(ec);
    }
}

void handle_connect(AMY_SYSTEM_NS::error_code const& ec,
                    amy::connector& connector)
{
    check_error(ec);
    std::cout << "Connected." << std::endl;
}
*/

std::string string_format(const std::string fmt_str, ...) {
    int final_n, n = ((int)fmt_str.size()) * 2; /* Reserve two times as much as the length of the fmt_str */
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while(1) {
        formatted.reset(new char[n]); /* Wrap the plain char array into the unique_ptr */
        strcpy(&formatted[0], fmt_str.c_str());
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}


void init() {
    cfg = new ConfigFile("cfg/config.json");

    string devID = cfg->Get("dev_id");
    string macAddress = cfg->Get("mac");
    string deviceKey = cfg->Get("device_key");

    Database db;
    if (db.connect(cfg->Get("db_host"), std::stoi(cfg->Get("db_port")), cfg->Get("user_name"), cfg->Get("pwd"), cfg->Get("db_name")))
    {
        std::cout << "ok......" << std::endl;
    }

    Table table(db);

    // select one record
    Row row;
    row.clear();
    row["mac_address"] = 30;
    row["device_key"] = "ping";
    table.from("parking_device").where("dev_id", devID).update(row);
    // if (row.empty())
    // {
    //     std::cout << "query result is empty" << std::endl;
    // }
    // else
    // {
    //     string mac = row["mac_address"];
    //     string token = row["token"];
    //     std::cout << "mac address:" << mac << " token=" << token << std::endl;
    // }


    connect_options options;
    options.server = cfg->Get("db_host");
    options.port = std::stoi(cfg->Get("db_port"));
    options.username = cfg->Get("user_name");
    options.password = cfg->Get("pwd");
    options.dbname = cfg->Get("db_name");

    if (!DBConnection.open(options) ) {
	    cout << "Connection failed" << endl;
    } else {
	    cout  << "db connection success" << endl;

        try {
            // string queryString = string_format("select dev_id, mac_address, device_key, token from parking_device where dev_id = `%s`", devID.c_str());
            string queryString = string_format("select dev_id, mac_address, device_key, token from `parking_device` WHERE `dev_id`='jetson01' LIMIT 0,1 ");
            cout << "query string:" << queryString << endl;
            DBConnection.query( queryString )
                .each([](string devid, string mac, string key, string token ) {
                    cout << "mac:" << mac << ",device key:" << key << ",token:" << token << endl;
                
                    return true;
                // string mac_address, device_key, token;
                // res.fetch(mac_address, device_key, token );
                // cout << "mac:" << mac_address << ",device key:" << device_key << ",token:" << token << endl;
            });

            DBConnection.exec("update `parking_device` set `mac_address` = `00:11:22:33:44:55`");

	    } catch (mysql_exception exp) {
    		cout << "Query #" << " failed with error: " << exp.error_number() << " - " << exp.what() << endl;
	    } catch (mysqlpp_exception exp) {
	    	cout << "Query #" << " failed with error: " << exp.what() << endl;
	    }

    }
    /*

    dataBaseConfig config;
    config.character_encoding = "utf8";
    config.conn_number = 2;
    config.dbname = cfg->Get("db_name");
    config.host = cfg->Get("db_host");
    config.port = std::stoi(cfg->Get("db_port"));
    config.password = cfg->Get("pwd");
    config.user =  cfg->Get("user_name");
    dao_t<mysql>::init_conn_pool(config);
    */


}

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
class MyUltAlprSdkParallelDeliveryCallback : public UltAlprSdkParallelDeliveryCallback
{
public:
    MyUltAlprSdkParallelDeliveryCallback(const std::string &charset) : m_strCharset(charset) {}
    virtual void onNewResult(const UltAlprSdkResult *result) const override
    {
        static size_t numParallelDeliveryResults = 0;
        ULTALPR_SDK_ASSERT(result != nullptr);
        const std::string &json = result->json();
        ULTALPR_SDK_PRINT_INFO("MyUltAlprSdkParallelDeliveryCallback::onNewResult(%d, %s, %zu): %s",
                               result->code(),
                               result->phrase(),
                               ++numParallelDeliveryResults,
                               !json.empty() ? json.c_str() : "{}");
    }

private:
    std::string m_strCharset;
};

void checkDeviceToken() {
    if(cfg) {
	// check device_key and mac exist ?
	bool need_upload = false;
	string device_key = cfg->Get("device_key");
	string mac = cfg->Get("mac");
	if( device_key.size() < 5 ) {
	    string jsonConfig;
	    string asstesFolder = "./assets";
	    jsonConfig += std::string("\"assets_folder\": \"") + asstesFolder.c_str() + std::string("\"");
	    jsonConfig = "{" + jsonConfig + "}";
	    // Initialize the engine
	    ULTALPR_SDK_ASSERT(UltAlprSdkEngine::init(jsonConfig.c_str()).isOK());

	    // Request runtime license key
	    const UltAlprSdkResult result = UltAlprSdkEngine::requestRuntimeLicenseKey(true);
	    if (result.isOK()) {
		cfg->Set("device_key", result.json() );
		cout << "request key result:" << result.json() << endl;
		need_upload = true;
	    }
	    else {
		ULTALPR_SDK_PRINT_ERROR("\n\n*** Failed: code -> %d, phrase -> %s ***\n\n",
			result.code(),
			result.phrase()
		);
	    }
	} else {
	    cout << "device key: " << device_key << " is exist" << endl;
	}

	if( mac.size() < 5 ) {
            macAddress mac_address;
            mac_address.getMac("eth0");
            cout << "mac address:" << mac_address.toString() << endl;
	    cfg->Set("mac", mac_address.toString() );
	    need_upload = true;
	}

	if( need_upload ) {
	    string devID = cfg->Get("dev_id");
	    string macAddress = cfg->Get("mac");
	    string deviceKey = cfg->Get("device_key");
        Database db;
        if (db.connect(cfg->Get("db_host"), std::stoi(cfg->Get("db_port")), cfg->Get("user_name"), cfg->Get("pwd"), cfg->Get("db_name")))
        {
            std::cout << "ok......" << std::endl;
        }

        Table table(db);

        // select one record
        Row row;
        row.clear();
        row["mac_address"] = macAddress;
        row["device_key"] = deviceKey;
        table.from("parking_device").where("dev_id", devID).update(row);

	    /*
	    try {
		auto res = dbConnection.query("select dev_id, mac_address, device_key, token from parking_device where dev_id = %s", devID.c_str() );
		if( !res.is_empty() ) {
		    string mac_address, device_key, token;
		    res.fetch(mac_address, device_key, token );
		    cout << "mac:" << mac_address << ",device key:" << device_key << ",token:" << token << endl;
		}

	    } catch (mysql_exception exp) {
		cout << "Query #" << " failed with error: " << exp.error_number() << " - " << exp.what() << endl;
	    } catch (mysqlpp_exception exp) {
		cout << "Query #" << " failed with error: " << exp.what() << endl;
	    }
	    */
	
	/*
	    dao_t<mysql> dao{ "xorm" };
	    string devID = cfg->Get("dev_id");
	    auto r = dao.query<parking_device>("where dev_id=?", devID);
	    if( !r.results.empty() ) {
		auto &info = r.results[0];
		info.mac_address = cfg->Get("mac");
		info.device_key = cfg->Get("device_key");
		cout << "info:" << info.mac_address << ",device key:" << info.device_key << endl;
		dao.update(info);
	    }
	*/
	}

    }
}

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
    //std::string licenseTokenData = "ANI6+wXQBUVDUFFVdzBBR1VQRkMuBApERW4nS1FTRkgvKTEAGTwQeEtZREJUdkdVV3tGCWl3akZOXFg4G3Q2Ymk7cUFYWygGCBQqDgZVSUUzPW5LaUUxRlUpImJcRkFkMRscJyQ6RlhxRTxODAtKNTE3MGRlRDFdVGZqVDc1fScXNH5IX1AlCzkjKRdRNUVbXGYMLz0/JigQBg5gVmNiW3oxeUtxCFU6I1J5WwsyXiEyGi1SQz0+QxgpJzIqXyxXV35eaDBSeU59Sn4VPgEGP2N6LzoeVls=";
	std::string licenseTokenData = cfg->Get("token");

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
	if (!assetsFolder.empty()) {
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
        cv::Mat frame = cv::Mat(cFrame->height(), cFrame->width(), CV_8UC3, (void*)cFrame->data());
        //cv::cvtColor(frame, displayFrame, CV_RGB2BGR);
        if( frame.ptr() ) {
            std::cout << "width:" << frame.size().width << ",height:" << frame.size().height << ",length:" << length << std::endl;
            frameQueue.enqueue(frame);
        }

        //cv::imshow("frame", displayFrame);
        //cv::waitKey(30);
        delete cFrame;
    }
}

void captureJob() {
    while(1) {
	macAddress mac;
	mac.getMac("eth0");
	cout << "mac address:" << mac.toString() << endl;

	/*
	ConfigFile conf("cfg/mysql.json");
	std::cout << "Get: " << conf.Get("pwd") << std::endl;

	connect_options options; 
	options.server = conf.Get("db_ip");
	options.port = 6033;
	options.username = conf.Get("user_name");
	options.password = conf.Get("pwd");
	options.dbname = conf.Get("db_name");

	connection my;
	if (!my.open(options) ) {
		cout << "Connection failed" << endl;
	}

	try {

		// Simple query and simple way to get back a single value:
		auto row_count = my.query("select count(*) from jreader")
							.get_value<int>();
		cout << "Count: " << row_count << endl;

	} catch (mysql_exception exp) {
		cout << "Query #" << " failed with error: " << exp.error_number() << " - " << exp.what() << endl;
	}
	catch (mysqlpp_exception exp) {
		cout << "Query #" << " failed with error: " << exp.what() << endl;
	}

        dataBaseConfig config;
        config.character_encoding = "utf8";
        config.conn_number = 2;
        config.dbname = "ndocar";
        config.host = "notice.com.tw";
	config.port = 6033;
        config.password = "qJlGgeg9uwAPPKlS";
        config.user = "facecam";
        dao_t<mysql>::init_conn_pool(config);
	*/


        cv::VideoCapture capture("rtsp://103.126.252.189:11011/stream0");
        // cv::VideoCapture capture("rtsp://114.34.177.189:11012/stream0");
        //cv::VideoCapture capture("alpr.mp4");
        capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

        cv::Mat frame;
        cout << "capture rtsp://103.126... capture stream into queue first" << endl;
        int count = 0;

        while (1) {        
            if (!capture.read(frame)) {
            	std::cout << "capture error, restart" << std::endl;
            	break;
	        } 
            // if(++count % 1000 == 0 ) {
            //     string filename = string_format("test_%d.jpg", count);
            //     cv::imwrite( filename, frame );
            //     if( frame.ptr() && frame.size().width > 0 && frame.size().height > 0 ) {
            //         uploadQueue.enqueue(filename);
            //         // sock.postUpload("http://notice.com.tw:2077/upload", filename, "file");
            //         // cout << "post " << filename << endl;
            //     }
            // }
	        frameQueue.enqueue(frame);
        }
    }
}

void handlePlateJob() {
    PlateInfo info;

    while(1) {
        if( plateQueue.try_dequeue(info) ) {
            if( info.plate.size() < 3 ) {
                continue;
            }
            replace( info.plate.begin(), info.plate.end(), 'I', '1' );
            replace( info.plate.begin(), info.plate.end(), 'O', '0' );

            // cout << "cx1:"<< info.cx1 << ",cx2:" << info.cx2 << ",cy1:" << info.cy1 << ",cy2:" << info.cy2 << endl;
            int width = info.cx2-info.cx1;
            int height = info.cy2 - info.cy1;
            if( width > 0 && height > 0 ) {
                cv::Rect car_region(info.cx1, info.cy1, width, height );
                cv::Mat carImg = info.frame(car_region);
                string filename = string_format("car_%s_%f.jpg", info.plate.c_str(), info.car_confidence);
                cv::imwrite( filename, carImg );
                if( carImg.ptr() && carImg.size().width > 0 && carImg.size().height > 0 ) {
                    uploadQueue.enqueue(filename);
                }
            }

            // cout << "plate:" << info.plate << ",px1:"<< info.px1 << ",px2:" << info.px2 << ",py1:" << info.py1 << ",py2:" << info.py2 << endl;
            width = info.px2 - info.px1;
            height = info.py2 - info.py1;
            if( width > 0 && height > 0 ) {
                cv::Rect plate_region(info.px1, info.py1, (info.px2-info.px1), (info.py2-info.py1) );
                cv::Mat plateImg = info.frame(plate_region);
                string filename = string_format("plate_%s_%f.jpg", info.plate.c_str(), info.plate_confidence1);
                cv::imwrite(filename, plateImg);
                if( plateImg.ptr() && plateImg.size().width > 0 && plateImg.size().height > 0 ) {
                    uploadQueue.enqueue(filename);
                }
            }
        }
    }
}

void uploadJob() {
    SSocket sock = SSocket();
    while(1) {
        string filename;
        if( uploadQueue.try_dequeue(filename) ) { 
            struct stat buffer;   
            if( stat(filename.c_str(), &buffer) == 0 ) {
                usleep(50000);
                //sock.postUpload("http://notice.com.tw:2077/upload", filename, "file");
                string command = string_format("curl -X POST http://notice.com.tw:2077/upload -F \"file=@%s\"", filename.c_str());
                system(command.c_str());
                cout << "command: " << command << endl;            }
        }

        usleep(10000);
    }
}

void connectRedisJob() {
    while(1) {
        RedisImageHelperSync clientSync(redisHost, redisPort, redisInputKey);
        if (!clientSync.connect()) {
            std::cerr << "Could not connect to redis server." << std::endl;
            usleep(50000);
            continue;
        }

        struct cameraParams contextData;
        contextData.width = clientSync.getInt(redisInputCameraParametersKey + ":width");
        contextData.height = clientSync.getInt(redisInputCameraParametersKey + ":height");
        contextData.channels = clientSync.getInt(redisInputCameraParametersKey + ":channels");

        if (STREAM_MODE) {
            RedisImageHelperAsync clientAsync(redisHost, redisPort, redisInputKey);
            if (!clientAsync.connect()) {
                std::cerr << "Could not connect to redis server." << std::endl;
                usleep(1000);
                continue;
            }
            clientAsync.subscribe(redisInputKey, onImagePublished, static_cast<void*>(&contextData));
            std::cout << "exit success" << std::endl;
            usleep(1000);
            continue;
        }
    }
}

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

    if (result.count("p1")) {
        webPort1 = result["p1"].as<int>();
        if (VERBOSE) {
            std::cerr << "Web port1 set to " << webPort1 << "." << std::endl;
        }
    }
    else {
        if (VERBOSE) {
            std::cerr << "No web port1 specified. Web port1 was set to " << webPort1 << "." << std::endl;
        }
    }

    if (result.count("p2")) {
        webPort1 = result["p2"].as<int>();
        if (VERBOSE) {
            std::cerr << "Web port2 set to " << webPort2 << "." << std::endl;
        }
    }
    else {
        if (VERBOSE) {
            std::cerr << "No web port2 specified. Web port1 was set to " << webPort2 << "." << std::endl;
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

void AlprProcess()
{
#ifdef ENABLE_ALPR
    checkDeviceToken();
    initUltimateEngine();
#endif

    thread plateThread( handlePlateJob );
    thread uploadThread( uploadJob );
///    thread connectRedisThread( connectRedisJob );
    thread captureThread( captureJob );

    cv::Mat frame;

    MJPEGWriter web(webPort1);
    web.start();

    while (1)
    {
        if( frameQueue.try_dequeue(frame) ) { 
#ifdef ENABLE_ALPR
            UltAlprSdkResult result = UltAlprSdkEngine::process(
                ULTALPR_SDK_IMAGE_TYPE_RGB24, // If you're using data from your camera then, the type would be YUV-family instead of RGB-family. https://www.doubango.org/SDKs/anpr/docs/cpp-api.html#_CPPv4N15ultimateAlprSdk22ULTALPR_SDK_IMAGE_TYPEE
                frame.ptr(),
                frame.size().width,
                frame.size().height);

            if (result.isOK() && result.json()) {
                const std::string &json_ = result.json();
                if (!json_.empty()) {
                    cout << "UltAlprSdkEngine result: " << json_.c_str() << ",found " << result.numCars() << " cars" << endl;
                    if( result.numCars() > 0 && result.numPlates() > 0) {
                        PlateInfo info;
                        json j = json::parse(json_);
                        if( j.contains("plates") ) {
                            json plates = j["plates"];
                            if( plates.is_array() ) {
                                for( int i=0; i<plates.size(); i++) {
                                    if( plates[i].contains("car") ) {
                                        json car = plates[i]["car"];
                                        json box = car["warpedBox"];
                                        float confidence = car["confidence"].get<float>();
                                        int cx1 = int(box[0].get<float>());
                                        int cy1 = int(box[1].get<float>());
                                        int cx2 = int(box[4].get<float>());
                                        int cy2 = int(box[5].get<float>());
                                        cout << "cx1:" << cx1 << ",cx2:" << cx2 << ",cy1:" << cy1 << ",cy2:" << cy2 << ",confidence:" << confidence << endl;

                                        cv::rectangle(frame, cv::Point(cx1, cy1), cv::Point(cx2, cy2), cv::Scalar(0, 255, 255), 2);
                                        info.car_confidence = confidence;
                                        info.cx1 = cx1;
                                        info.cx2 = cx2;
                                        info.cy1 = cy1;
                                        info.cy2 = cy2;
                                        info.frame = frame;
                                        info.plate = "";
                                    }

                                    std::string plate = plates[i]["text"].get<std::string>();
                                    json confidences = plates[i]["confidences"];
                                    float confidence0,confidence1;
                                    if( confidences.is_array() ) {
                                        confidence0 = confidences[0].get<float>();
                                        confidence1 = confidences[1].get<float>();
                                    }

                                    std::cout << "plate:" << plate << ",confidences:" << confidences << std::endl;
                                    json box = plates[i]["warpedBox"];
                                    int px1 = int(box[0].get<float>());
                                    int py1 = int(box[1].get<float>());
                                    int px2 = int(box[4].get<float>());
                                    int py2 = int(box[5].get<float>());

                                    cv::rectangle(frame, cv::Point(px1, py1), cv::Point(px2, py2),  cv::Scalar(255, 255, 0), 2 );
                                    info.plate_confidence0 = confidence0;
                                    info.plate_confidence1 = confidence1;
                                    info.px1 = px1;
                                    info.px2 = px2;
                                    info.py1 = py1;
                                    info.py2 = py2;
                                    info.plate = plate;
                                }
                            }
                        }
                        plateQueue.enqueue(info);
                    }
                }
            }
#endif
            web.write(frame);
        }
    }

#ifdef ENABLE_ALPR
    UltAlprSdkResult result = UltAlprSdkEngine::deInit();
#endif

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
    init();

    AlprProcess();

}

