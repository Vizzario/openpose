#ifndef OPENPOSE_PYTHON_HPP
#define OPENPOSE_PYTHON_HPP
#define BOOST_DATE_TIME_NO_LIB

// OpenPose dependencies
#include <openpose/core/headers.hpp>
#include <openpose/filestream/headers.hpp>
#include <openpose/gui/headers.hpp>
#include <openpose/pose/headers.hpp>
#include <openpose/utilities/headers.hpp>
#include <caffe/caffe.hpp>
#include <stdlib.h>

#include <openpose/net/bodyPartConnectorCaffe.hpp>
#include <openpose/net/nmsCaffe.hpp>
#include <openpose/net/resizeAndMergeCaffe.hpp>
#include <openpose/pose/poseParameters.hpp>
#include <openpose/pose/enumClasses.hpp>
#include <openpose/pose/poseExtractor.hpp>
#include <openpose/gpu/cuda.hpp>
#include <openpose/gpu/opencl.hcl>
#include <openpose/core/macros.hpp>
#include <openpose/utilities/json.hpp>

#include <openpose/flags.hpp>
#include <openpose/wrapper/headers.hpp>

#ifdef _WIN32
    #define OP_EXPORT __declspec(dllexport)
#else
    #define OP_EXPORT
#endif

OP_API class OpenPose {
public:
    std::unique_ptr<op::Wrapper> opWrapper;

    std::unique_ptr<op::PoseExtractorCaffe> poseExtractorCaffe;
    std::unique_ptr<op::PoseCpuRenderer> poseRenderer;
    std::unique_ptr<op::FrameDisplayer> frameDisplayer;
    std::unique_ptr<op::ScaleAndSizeExtractor> scaleAndSizeExtractor;

    std::unique_ptr<op::ResizeAndMergeCaffe<float>> resizeAndMergeCaffe;
    std::unique_ptr<op::NmsCaffe<float>> nmsCaffe;
    std::unique_ptr<op::BodyPartConnectorCaffe<float>> bodyPartConnectorCaffe;
    std::shared_ptr<caffe::Blob<float>> heatMapsBlob;
    std::shared_ptr<caffe::Blob<float>> peaksBlob;
    op::Array<float> mPoseKeypoints;
    op::Array<float> mPoseScores;
    op::PoseModel poseModel;
    int mGpuID;

    OpenPose(json::JSON jsonParams)
    {
        // Setup variables
        if(jsonParams.hasKey("write_json")) FLAGS_write_json = jsonParams["write_json"].ToString();
        if(jsonParams.hasKey("num_gpu")) FLAGS_num_gpu = jsonParams["num_gpu"].ToInt();
        if(jsonParams.hasKey("num_gpu_start")) FLAGS_num_gpu_start = jsonParams["num_gpu_start"].ToInt();
        if(jsonParams.hasKey("logging_level")) FLAGS_logging_level = jsonParams["logging_level"].ToInt();
        if(jsonParams.hasKey("output_resolution")) FLAGS_output_resolution = jsonParams["output_resolution"].ToString();
        if(jsonParams.hasKey("net_resolution")) FLAGS_net_resolution = jsonParams["net_resolution"].ToString();
        if(jsonParams.hasKey("model_pose")) FLAGS_model_pose = jsonParams["model_pose"].ToString();
        if(jsonParams.hasKey("alpha_pose")) FLAGS_alpha_pose = jsonParams["alpha_pose"].ToFloat();
        if(jsonParams.hasKey("scale_gap")) FLAGS_scale_gap = jsonParams["scale_gap"].ToFloat();
        if(jsonParams.hasKey("scale_number")) FLAGS_scale_number = jsonParams["scale_number"].ToInt();
        if(jsonParams.hasKey("render_threshold")) FLAGS_render_threshold = jsonParams["render_threshold"].ToFloat();
        if(jsonParams.hasKey("disable_blending")) FLAGS_disable_blending = jsonParams["disable_blending"].ToInt();
        if(jsonParams.hasKey("model_folder")) FLAGS_model_folder = jsonParams["model_folder"].ToString();
        if(jsonParams.hasKey("body_disable")) FLAGS_body_disable = jsonParams["body_disable"].ToBool();
        if(jsonParams.hasKey("keypoint_scale")) FLAGS_keypoint_scale = jsonParams["keypoint_scale"].ToInt();
        if(jsonParams.hasKey("render_pose")) FLAGS_render_pose = jsonParams["render_pose"].ToInt();
        if(jsonParams.hasKey("alpha_heatmap")) FLAGS_alpha_heatmap = jsonParams["alpha_heatmap"].ToFloat();
        if(jsonParams.hasKey("part_to_show")) FLAGS_part_to_show = jsonParams["part_to_show"].ToInt();
        if(jsonParams.hasKey("heatmaps_add_parts")) FLAGS_heatmaps_add_parts = jsonParams["heatmaps_add_parts"].ToBool();
        if(jsonParams.hasKey("heatmaps_add_bkg")) FLAGS_heatmaps_add_bkg = jsonParams["heatmaps_add_bkg"].ToBool();
        if(jsonParams.hasKey("heatmaps_add_PAFs")) FLAGS_heatmaps_add_PAFs = jsonParams["heatmaps_add_PAFs"].ToBool();
        if(jsonParams.hasKey("heatmaps_scale")) FLAGS_heatmaps_scale = jsonParams["heatmaps_scale"].ToInt();
        if(jsonParams.hasKey("part_candidates")) FLAGS_part_candidates = jsonParams["part_candidates"].ToBool();
        if(jsonParams.hasKey("number_people_max")) FLAGS_number_people_max = jsonParams["number_people_max"].ToInt();
        // Face
        if(jsonParams.hasKey("face")) FLAGS_face = jsonParams["face"].ToBool();
        if(jsonParams.hasKey("face_net_resolution")) FLAGS_face_net_resolution = jsonParams["face_net_resolution"].ToString();
        if(jsonParams.hasKey("face_render")) FLAGS_face_render = jsonParams["face_render"].ToInt();
        if(jsonParams.hasKey("face_alpha_pose")) FLAGS_face_alpha_pose = jsonParams["face_alpha_pose"].ToFloat();
        if(jsonParams.hasKey("face_alpha_heatmap")) FLAGS_face_alpha_heatmap = jsonParams["face_alpha_heatmap"].ToFloat();
        if(jsonParams.hasKey("face_render_threshold")) FLAGS_face_render_threshold = jsonParams["face_render_threshold"].ToFloat();
        // Hands
        if(jsonParams.hasKey("hand")) FLAGS_hand = jsonParams["hand"].ToBool();
        if(jsonParams.hasKey("hand_net_resolution")) FLAGS_hand_net_resolution = jsonParams["hand_net_resolution"].ToString();
        if(jsonParams.hasKey("hand_scale_number")) FLAGS_hand_scale_number = jsonParams["hand_scale_number"].ToInt();
        if(jsonParams.hasKey("hand_scale_range")) FLAGS_hand_scale_range = jsonParams["hand_scale_range"].ToFloat();
        if(jsonParams.hasKey("hand_render")) FLAGS_hand_render = jsonParams["hand_render"].ToInt();
        if(jsonParams.hasKey("hand_alpha_pose")) FLAGS_hand_alpha_pose = jsonParams["hand_alpha_pose"].ToFloat();
        if(jsonParams.hasKey("hand_alpha_heatmap")) FLAGS_hand_alpha_heatmap = jsonParams["hand_alpha_heatmap"].ToFloat();
        if(jsonParams.hasKey("hand_render_threshold")) FLAGS_hand_render_threshold = jsonParams["hand_render_threshold"].ToFloat();

        // GPU Setting
        mGpuID = FLAGS_num_gpu_start;
        #ifdef USE_CUDA
        caffe::Caffe::set_mode(caffe::Caffe::GPU);
        caffe::Caffe::SetDevice(mGpuID);
        #elif USE_OPENCL
        FLAGS_render_pose = 1;
        caffe::Caffe::set_mode(caffe::Caffe::GPU);
        std::vector<int> devices;
        const int maxNumberGpu = op::OpenCL::getTotalGPU();
        for (auto i = 0; i < maxNumberGpu; i++)
            devices.emplace_back(i);
        caffe::Caffe::SetDevices(devices);
        caffe::Caffe::SelectDevice(mGpuID, true);
        op::OpenCL::getInstance(mGpuID, CL_DEVICE_TYPE_GPU, true);
        #else
        FLAGS_render_pose = 0;
        caffe::Caffe::set_mode(caffe::Caffe::CPU);
        #endif
        op::log("OpenPose Library Python Wrapper", op::Priority::High);

        // Logging_level
        op::check(0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.",
                  __LINE__, __FUNCTION__, __FILE__);
        op::ConfigureLog::setPriorityThreshold((op::Priority)FLAGS_logging_level);
        op::Profiler::setDefaultX(FLAGS_profile_speed);

        // Applying user defined configuration - GFlags to program variables
        const auto outputSize = op::flagsToPoint(FLAGS_output_resolution, "-1x-1");
        const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x368");
        const auto faceNetInputSize = op::flagsToPoint(FLAGS_face_net_resolution, "368x368 (multiples of 16)");
        const auto handNetInputSize = op::flagsToPoint(FLAGS_hand_net_resolution, "368x368 (multiples of 16)");
        const auto poseModel = op::flagsToPoseModel(FLAGS_model_pose);
        if (!FLAGS_write_keypoint.empty())
            op::log("Flag `write_keypoint` is deprecated and will eventually be removed."
                    " Please, use `write_json` instead.", op::Priority::Max);
        const auto keypointScale = op::flagsToScaleMode(FLAGS_keypoint_scale);
        const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg,
                                                      FLAGS_heatmaps_add_PAFs);
        const auto heatMapScale = op::flagsToHeatMapScaleMode(FLAGS_heatmaps_scale);
        const auto multipleView = (FLAGS_3d || FLAGS_3d_views > 1);
        const bool enableGoogleLogging = true;

        // OP Wrapper Object
        opWrapper = std::unique_ptr<op::Wrapper>(new op::Wrapper {op::ThreadManagerMode::Asynchronous});

        // Pose configuration (use WrapperStructPose{} for default and recommended configuration)
        const auto FLAGS_render_mode = op::RenderMode::None;
        const op::WrapperStructPose wrapperStructPose{
            !FLAGS_body_disable, netInputSize, outputSize, keypointScale, FLAGS_num_gpu, FLAGS_num_gpu_start,
            FLAGS_scale_number, (float)FLAGS_scale_gap, FLAGS_render_mode,
            poseModel, !FLAGS_disable_blending, (float)FLAGS_alpha_pose, (float)FLAGS_alpha_heatmap,
            FLAGS_part_to_show, FLAGS_model_folder, heatMapTypes, heatMapScale, FLAGS_part_candidates,
            (float)FLAGS_render_threshold, FLAGS_number_people_max, enableGoogleLogging};
        opWrapper->configure(wrapperStructPose);

        // Hands configuration
        const op::WrapperStructHand wrapperStructHand{
            FLAGS_hand, handNetInputSize, FLAGS_hand_scale_number, (float)FLAGS_hand_scale_range, FLAGS_hand_tracking,
            FLAGS_render_mode, (float)FLAGS_hand_alpha_pose,
            (float)FLAGS_hand_alpha_heatmap, (float)FLAGS_hand_render_threshold};
        // if(FLAGS_hand) opWrapper->configure(wrapperStructHand);
        opWrapper->configure(wrapperStructHand);

        // Face configuration (use op::WrapperStructFace{} to disable it)
        const op::WrapperStructFace wrapperStructFace{
            FLAGS_face, faceNetInputSize, FLAGS_render_mode,
            (float)FLAGS_face_alpha_pose, (float)FLAGS_face_alpha_heatmap, (float)FLAGS_face_render_threshold};
        // if(FLAGS_face) opWrapper->configure(wrapperStructFace);
        opWrapper->configure(wrapperStructFace);

        // Output configuration
        const auto displayMode = op::DisplayMode::NoDisplay;
        const bool guiVerbose = false;
        const bool fullScreen = false;
        const op::WrapperStructOutput wrapperStructOutput{
            displayMode, guiVerbose, fullScreen, FLAGS_write_keypoint,
            op::stringToDataFormat(FLAGS_write_keypoint_format), FLAGS_write_json, FLAGS_write_coco_json,
            FLAGS_write_coco_foot_json, FLAGS_write_images, FLAGS_write_images_format, FLAGS_write_video,
            FLAGS_camera_fps, FLAGS_write_heatmaps, FLAGS_write_heatmaps_format, FLAGS_write_video_adam,
            FLAGS_write_bvh, FLAGS_udp_host, FLAGS_udp_port};
        opWrapper->configure(wrapperStructOutput);

        // Start wrapper
        opWrapper->disableMultiThreading();
        opWrapper->start();

        // Step 3 - Initialize all required classes
        scaleAndSizeExtractor = std::unique_ptr<op::ScaleAndSizeExtractor>(new op::ScaleAndSizeExtractor(netInputSize, outputSize, FLAGS_scale_number, FLAGS_scale_gap));
        poseExtractorCaffe = std::unique_ptr<op::PoseExtractorCaffe>(new op::PoseExtractorCaffe{ poseModel, FLAGS_model_folder, FLAGS_num_gpu_start });
        poseRenderer = std::unique_ptr<op::PoseCpuRenderer>(new op::PoseCpuRenderer{ poseModel, (float)FLAGS_render_threshold, !FLAGS_disable_blending,
            (float)FLAGS_alpha_pose });
        frameDisplayer = std::unique_ptr<op::FrameDisplayer>(new op::FrameDisplayer{ "OpenPose Tutorial - Example 1", outputSize });

        // Custom
        resizeAndMergeCaffe = std::unique_ptr<op::ResizeAndMergeCaffe<float>>(new op::ResizeAndMergeCaffe<float>{});
        nmsCaffe = std::unique_ptr<op::NmsCaffe<float>>(new op::NmsCaffe<float>{});
        bodyPartConnectorCaffe = std::unique_ptr<op::BodyPartConnectorCaffe<float>>(new op::BodyPartConnectorCaffe<float>{});
        heatMapsBlob = { std::make_shared<caffe::Blob<float>>(1,1,1,1) };
        peaksBlob = { std::make_shared<caffe::Blob<float>>(1,1,1,1) };
        bodyPartConnectorCaffe->setPoseModel(poseModel);

        // Step 4 - Initialize resources on desired thread (in this case single thread, i.e. we init resources here)
        //poseExtractorCaffe->initializationOnThread();
        poseRenderer->initializationOnThread();
    }

    void forward(const cv::Mat& inputImage,
                 op::Array<float>& poseKeypoints, op::Array<float>& leftHandKeypoints,
                 op::Array<float>& rightHandKeypoints, op::Array<float>& faceKeypoints,
                 cv::Mat& displayImage, bool display = false)
    {
        auto datumProcessed = opWrapper->emplaceAndPop(inputImage);
        displayImage = datumProcessed->at(0).cvOutputData;
        poseKeypoints = datumProcessed->at(0).poseKeypoints;
        leftHandKeypoints = datumProcessed->at(0).handKeypoints[0];
        rightHandKeypoints = datumProcessed->at(0).handKeypoints[1];
        faceKeypoints = datumProcessed->at(0).faceKeypoints;
    }

    std::vector<caffe::Blob<float>*> caffeNetSharedToPtr(
        std::vector<boost::shared_ptr<caffe::Blob<float>>>& caffeNetOutputBlob)
    {
        try
        {
            // Prepare spCaffeNetOutputBlobss
            std::vector<caffe::Blob<float>*> caffeNetOutputBlobs(caffeNetOutputBlob.size());
            for (auto i = 0u; i < caffeNetOutputBlobs.size(); i++)
                caffeNetOutputBlobs[i] = caffeNetOutputBlob[i].get();
            return caffeNetOutputBlobs;
        }
        catch (const std::exception& e)
        {
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return{};
        }
    }

    void poseFromHeatmap(const cv::Mat& inputImage, std::vector<boost::shared_ptr<caffe::Blob<float>>>& caffeNetOutputBlob, op::Array<float>& poseKeypoints, cv::Mat& displayImage, std::vector<op::Point<int>>& imageSizes) {
        // Get Scale
        const op::Point<int> inputDataSize{ inputImage.cols, inputImage.rows };

        // Convert to Ptr
        //std::vector<boost::shared_ptr<caffe::Blob<float>>> a;
        //caffeNetOutputBlob.emplace_back(caffeHmPtr);
        const auto caffeNetOutputBlobs = caffeNetSharedToPtr(caffeNetOutputBlob);

        // To be called once only
        resizeAndMergeCaffe->Reshape(caffeNetOutputBlobs, { heatMapsBlob.get() },
            op::getPoseNetDecreaseFactor(poseModel), 1.f / 1.f, true,
            0);
        nmsCaffe->Reshape({ heatMapsBlob.get() }, { peaksBlob.get() }, op::getPoseMaxPeaks(poseModel),
            op::getPoseNumberBodyParts(poseModel), 0);
        bodyPartConnectorCaffe->Reshape({ heatMapsBlob.get(), peaksBlob.get() });

        // Normal
        op::OpOutputToCvMat opOutputToCvMat;
        op::CvMatToOpInput cvMatToOpInput;
        op::CvMatToOpOutput cvMatToOpOutput;
        if (inputImage.empty())
            op::error("Could not open or find the image: ", __LINE__, __FUNCTION__, __FILE__);
        const op::Point<int> imageSize{ inputImage.cols, inputImage.rows };
        // Step 2 - Get desired scale sizes
        std::vector<double> scaleInputToNetInputs;
        std::vector<op::Point<int>> netInputSizes;
        double scaleInputToOutput;
        op::Point<int> outputResolution;

        std::tie(scaleInputToNetInputs, netInputSizes, scaleInputToOutput, outputResolution)
            = scaleAndSizeExtractor->extract(imageSize);

        const auto netInputArray = cvMatToOpInput.createArray(inputImage, scaleInputToNetInputs, netInputSizes);

        // Run the modes
        const std::vector<float> floatScaleRatios(scaleInputToNetInputs.begin(), scaleInputToNetInputs.end());
        resizeAndMergeCaffe->setScaleRatios(floatScaleRatios);
        std::vector<caffe::Blob<float>*> heatMapsBlobs{ heatMapsBlob.get() };
        std::vector<caffe::Blob<float>*> peaksBlobs{ peaksBlob.get() };
#ifdef USE_CUDA
        resizeAndMergeCaffe->Forward_gpu(caffeNetOutputBlobs, heatMapsBlobs); // ~5ms
#elif USE_OPENCL
        resizeAndMergeCaffe->Forward_ocl(caffeNetOutputBlobs, heatMapsBlobs); // ~5ms
#else
        resizeAndMergeCaffe->Forward_cpu(caffeNetOutputBlobs, heatMapsBlobs); // ~5ms
#endif

        nmsCaffe->setThreshold((float)poseExtractorCaffe->get(op::PoseProperty::NMSThreshold));
#ifdef USE_CUDA
        nmsCaffe->Forward_gpu(heatMapsBlobs, peaksBlobs);// ~2ms
#elif USE_OPENCL
        nmsCaffe->Forward_ocl(heatMapsBlobs, peaksBlobs);// ~2ms
#else
        nmsCaffe->Forward_cpu(heatMapsBlobs, peaksBlobs);// ~2ms
#endif
        op::cudaCheck(__LINE__, __FUNCTION__, __FILE__);

        float mScaleNetToOutput = 1. / scaleInputToNetInputs[0];
        bodyPartConnectorCaffe->setScaleNetToOutput(mScaleNetToOutput);
        bodyPartConnectorCaffe->setInterMinAboveThreshold(
            (float)poseExtractorCaffe->get(op::PoseProperty::ConnectInterMinAboveThreshold)
        );
        bodyPartConnectorCaffe->setInterThreshold((float)poseExtractorCaffe->get(op::PoseProperty::ConnectInterThreshold));
        bodyPartConnectorCaffe->setMinSubsetCnt((int)poseExtractorCaffe->get(op::PoseProperty::ConnectMinSubsetCnt));
        bodyPartConnectorCaffe->setMinSubsetScore((float)poseExtractorCaffe->get(op::PoseProperty::ConnectMinSubsetScore));

#ifdef USE_CUDA
        bodyPartConnectorCaffe->Forward_gpu({ heatMapsBlob.get(),
            peaksBlob.get() },
            mPoseKeypoints, mPoseScores);
#else
        bodyPartConnectorCaffe->Forward_cpu({ heatMapsBlob.get(),
            peaksBlob.get() },
            mPoseKeypoints, mPoseScores);
#endif
        poseKeypoints = mPoseKeypoints;

        auto outputArray = cvMatToOpOutput.createArray(inputImage, scaleInputToOutput, outputResolution);
        // Step 5 - Render poseKeypoints
        poseRenderer->renderPose(outputArray, mPoseKeypoints, scaleInputToOutput);
        // Step 6 - OpenPose output format to cv::Mat
        displayImage = opOutputToCvMat.formatToCvMat(outputArray);
    }
};

#ifdef __cplusplus
extern "C" {
#endif
    typedef void* c_OP;
    op::Array<float> poseKeypoints, leftHandKeypoints, rightHandKeypoints, faceKeypoints;

    void populateSize(const op::Array<float>& keypoints, int* size){
        if (keypoints.getSize().size()) {
            size[0] = keypoints.getSize()[0];
            size[1] = keypoints.getSize()[1];
            size[2] = keypoints.getSize()[2];
        }
        else {
            size[0] = 0; size[1] = 0; size[2] = 0;
        }
    }
    OP_EXPORT c_OP newOP(char* jsonParamsString) {
        json::JSON jsonParams = json::JSON::Load(jsonParamsString);
        return new OpenPose(jsonParams);
    }
    OP_EXPORT void delOP(c_OP op) {
        delete (OpenPose *)op;
    }
    OP_EXPORT void forward(c_OP op, unsigned char* img, size_t rows, size_t cols, int* poseSize, int* leftHandSize, int* rightHandSize, int* faceSize, unsigned char* displayImg, bool display) {
        OpenPose* openPose = (OpenPose*)op;
        cv::Mat image(rows, cols, CV_8UC3, img);
        cv::Mat displayImage(rows, cols, CV_8UC3, displayImg);
        openPose->forward(image, poseKeypoints, leftHandKeypoints, rightHandKeypoints, faceKeypoints, displayImage, display);
        populateSize(poseKeypoints, poseSize);
        populateSize(leftHandKeypoints, leftHandSize);
        populateSize(rightHandKeypoints, rightHandSize);
        populateSize(faceKeypoints, faceSize);
        if (display) memcpy(displayImg, displayImage.ptr(), sizeof(unsigned char)*rows*cols * 3);
    }
    OP_EXPORT void getPoseOutputs(c_OP op, float* array) {
        if (poseKeypoints.getSize().size())
            memcpy(array, poseKeypoints.getPtr(), poseKeypoints.getSize()[0] * poseKeypoints.getSize()[1] * poseKeypoints.getSize()[2] * sizeof(float));
    }
    OP_EXPORT void getLeftHandOutputs(c_OP op, float* array) {
        if (leftHandKeypoints.getSize().size())
            memcpy(array, leftHandKeypoints.getPtr(), leftHandKeypoints.getSize()[0] * leftHandKeypoints.getSize()[1] * leftHandKeypoints.getSize()[2] * sizeof(float));
    }
    OP_EXPORT void getRightHandOutputs(c_OP op, float* array) {
        if (rightHandKeypoints.getSize().size())
            memcpy(array, rightHandKeypoints.getPtr(), rightHandKeypoints.getSize()[0] * rightHandKeypoints.getSize()[1] * rightHandKeypoints.getSize()[2] * sizeof(float));
    }
    OP_EXPORT void getFaceOutputs(c_OP op, float* array) {
        if (faceKeypoints.getSize().size())
            memcpy(array, faceKeypoints.getPtr(), faceKeypoints.getSize()[0] * faceKeypoints.getSize()[1] * faceKeypoints.getSize()[2] * sizeof(float));
    }
    OP_EXPORT void poseFromHeatmap(c_OP op, unsigned char* img, size_t rows, size_t cols, unsigned char* displayImg, float* hm, int* size, float* ratios) {
        OpenPose* openPose = (OpenPose*)op;
        cv::Mat image(rows, cols, CV_8UC3, img);
        cv::Mat displayImage(rows, cols, CV_8UC3, displayImg);

        std::vector<boost::shared_ptr<caffe::Blob<float>>> caffeNetOutputBlob;

        for (int i = 0; i<size[0]; i++) {
            boost::shared_ptr<caffe::Blob<float>> caffeHmPtr(new caffe::Blob<float>());
            caffeHmPtr->Reshape(1, size[1], size[2] * ((float)ratios[i] / (float)ratios[0]), size[3] * ((float)ratios[i] / (float)ratios[0]));
            float* startIndex = &hm[i*size[1] * size[2] * size[3]];
            for (int d = 0; d<caffeHmPtr->shape()[1]; d++) {
                for (int r = 0; r<caffeHmPtr->shape()[2]; r++) {
                    for (int c = 0; c<caffeHmPtr->shape()[3]; c++) {
                        int toI = d*caffeHmPtr->shape()[2] * caffeHmPtr->shape()[3] + r*caffeHmPtr->shape()[3] + c;
                        int fromI = d*size[2] * size[3] + r*size[3] + c;
                        caffeHmPtr->mutable_cpu_data()[toI] = startIndex[fromI];
                    }
                }
            }
            caffeNetOutputBlob.emplace_back(caffeHmPtr);
        }

        std::vector<op::Point<int>> imageSizes;
        for (int i = 0; i<size[0]; i++) {
            op::Point<int> point(cols*ratios[i], rows*ratios[i]);
            imageSizes.emplace_back(point);
        }

        openPose->poseFromHeatmap(image, caffeNetOutputBlob, poseKeypoints, displayImage, imageSizes);
        memcpy(displayImg, displayImage.ptr(), sizeof(unsigned char)*rows*cols * 3);
        // Copy back kp size
        if (poseKeypoints.getSize().size()) {
            size[0] = poseKeypoints.getSize()[0];
            size[1] = poseKeypoints.getSize()[1];
            size[2] = poseKeypoints.getSize()[2];
        }
        else {
            size[0] = 0; size[1] = 0; size[2] = 0;
        }
    }

#ifdef __cplusplus
}
#endif

#endif
