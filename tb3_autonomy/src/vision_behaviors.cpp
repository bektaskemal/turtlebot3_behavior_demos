#include "vision_behaviors.h"

#include <ros/ros.h>
#include "sensor_msgs/Image.h"
#include <image_transport/image_transport.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d.hpp>
#include <cv_bridge/cv_bridge.h>

LookForObject::LookForObject(const std::string& name) :
    BT::SyncActionNode(name, {})
{
    std::cout << "[" << this->name() << "] Initialized" << std::endl;
}

void LookForObject::init(const ros::NodeHandle& nh) {
    nh_ = nh;
}

BT::NodeStatus LookForObject::tick()
{
    std::cout << "[" << this->name() << "] Looking for object" << std::endl;
    
    // Receive an image
    cv::namedWindow("Image");
    image_transport::ImageTransport it(nh_);
    image_transport::Subscriber sub = it.subscribe("/camera/rgb/image_raw", 1, &LookForObject::imageCallback, this);
    ros::Duration(1.0).sleep();
    received_image_ = false;
    while (!received_image_) {
        ros::spinOnce();
    }
    sub.shutdown();

    // Convert to HSV and threshold
    // TODO: Promote color threshold parameters based on a separate input
    cv::Mat img, img_hsv, img_threshold, img_keypoints;
    img = cv_bridge::toCvShare(latest_image_, "bgr8")->image;
    cv::cvtColor(img, img_hsv, cv::COLOR_BGR2HSV);
    cv::inRange(img_hsv, 
        cv::Scalar(40, 220, 0), 
        cv::Scalar(80, 255, 255), img_threshold);

    // Do blob detection
    cv::SimpleBlobDetector::Params params;
    params.minArea = 100;
    params.maxArea = 100000;
    params.filterByArea = true;
    params.filterByColor = false;
    params.filterByInertia = false;
    params.filterByConvexity = false;
    params.thresholdStep = 50;
    cv::Ptr<cv::SimpleBlobDetector> detector = cv::SimpleBlobDetector::create(params);
    std::vector<cv::KeyPoint> keypoints;
    detector->detect(img_threshold, keypoints);
    cv::drawKeypoints(img, keypoints, img_keypoints, 
        cv::Scalar(255,0,0), 
        cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

    // Display the image
    cv::imshow("Image", img_keypoints);
    cv::waitKey(2000);

    if (keypoints.size() > 0) {
        std::cout << "[" << this->name() << "] Found object" << std::endl;
        return BT::NodeStatus::SUCCESS;
    } else {
        std::cout << "[" << this->name() << "] No object detected" << std::endl;
        return BT::NodeStatus::FAILURE;
    }
}

void LookForObject::imageCallback(const sensor_msgs::ImageConstPtr& msg) {
    latest_image_ = msg;
    received_image_ = true;
}