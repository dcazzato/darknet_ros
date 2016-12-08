#include "ros_interface.h"
#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <sensor_msgs/Image.h>
#include <geometry_msgs/Point.h>
#include <vector>
#include <iostream>
#include <pthread.h>
#include <std_msgs/Int8.h>
#include <math.h>
#include <darknet_rsl/bbox_array.h>
#include <darknet_rsl/bbox.h>
#include <string>

extern "C" {
  #include "box.h"
}

// initialize YOLO functions that are called in this script
extern "C" RosBox_ *demo_yolo();
extern "C" void load_network(char *cfgfile, char *weightfile, float thresh);

const std::string classLabels_[] = { "aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat",
		     	             "chair", "cow", "dining table", "dog", "horse", "motorbike", "person",
		                     "potted plant", "sheep", "sofa", "train", "tv monitor" };
const int numClasses_ = sizeof(classLabels_)/sizeof(classLabels_[0]);

cv::Mat camImageCopy_;

// define parameters
std::string cameraTopicName_;
const std::string opencvWindow_ = "YOLO object detection";
int frameWidth_;
int frameHeight_;
int frameArea_;
int frameCount_ = 0;

// define a function that will replace CvVideoCapture.
// This function is called in yolo_kernels and allows YOLO to receive the ROS image
// message as an IplImage
IplImage* get_ipl_image()
{
   IplImage* ROS_img = new IplImage(camImageCopy_);
   return ROS_img;
}

class YoloObjectDetector
{
   ros::NodeHandle nodeHandle_;
   image_transport::ImageTransport imageTransport_;
   image_transport::Subscriber imageSubscriber_;
   ros::Publisher objectPublisher_;
   ros::Publisher bboxesPublisher_;
   std::vector< std::vector<RosBox_> > rosBoxes_;
   std::vector<int> rosBoxCounter_;
   std::vector<cv::Scalar> rosBoxColors_;
   darknet_rsl::bbox_array bboxesResults_;
   RosBox_* boxes_;

public:
   YoloObjectDetector() : imageTransport_(nodeHandle_), rosBoxes_(numClasses_), rosBoxCounter_(numClasses_, 0), rosBoxColors_(numClasses_)
   {
      int incr = floor(255/numClasses_);
      for (int i = 0; i < numClasses_; i++)
      {
         rosBoxColors_[i] = cv::Scalar(255 - incr*i, 0 + incr*i, 255 - incr*i);
      }

      imageSubscriber_ = imageTransport_.subscribe(cameraTopicName_, 1,
	                       &YoloObjectDetector::cameraCallback,this);
      objectPublisher_ = nodeHandle_.advertise<std_msgs::Int8>("found_object", 1);
      bboxesPublisher_ = nodeHandle_.advertise<darknet_rsl::bbox_array>("YOLO_bboxes", 1);

      cv::namedWindow(opencvWindow_, cv::WINDOW_NORMAL);
   }

   ~YoloObjectDetector()
   {
      cv::destroyWindow(opencvWindow_);
   }

private:
   void drawBoxes(cv::Mat &inputFrame, std::vector<RosBox_> &rosBoxes, int &numberOfObjects,
		   cv::Scalar &rosBoxColor, const std::string &objectLabel)
   {
      darknet_rsl::bbox bbox_result;

      for (int i = 0; i < numberOfObjects; i++)
      {
         int xmin = (rosBoxes[i].x - rosBoxes[i].w/2)*frameWidth_;
         int ymin = (rosBoxes[i].y - rosBoxes[i].h/2)*frameHeight_;
         int xmax = (rosBoxes[i].x + rosBoxes[i].w/2)*frameWidth_;
         int ymax = (rosBoxes[i].y + rosBoxes[i].h/2)*frameHeight_;

         bbox_result.Class = objectLabel;
         bbox_result.probability = rosBoxes[i].prob;
         bbox_result.xmin = xmin;
         bbox_result.ymin = ymin;
         bbox_result.xmax = xmax;
         bbox_result.ymax = ymax;
         bboxesResults_.bboxes.push_back(bbox_result);

         // draw bounding box of first object found
         cv::Point topLeftCorner = cv::Point(xmin, ymin);
         cv::Point botRightCorner = cv::Point(xmax, ymax);
         cv::rectangle(inputFrame, topLeftCorner, botRightCorner, rosBoxColor, 2);
         std::ostringstream probability;
         probability << rosBoxes[i].prob;
         cv::putText(inputFrame, objectLabel + " (" + probability.str() + ")", cv::Point(xmin, ymax+15), cv::FONT_HERSHEY_PLAIN,
                     1.0, rosBoxColor, 2.0);
      }
   }

   void runYolo(cv::Mat &fullFrame)
   {
      cv::Mat input_frame = fullFrame.clone();

      // run yolo and get bounding boxes for objects
      boxes_ = demo_yolo();

      // get the number of bounding boxes found
      int num = boxes_[0].num;

      // if at least one bbox found, draw box
      if (num > 0  && num <= 100)
      {
        std::cout << "# Objects: " << num << std::endl;

        // split bounding boxes by class
       for (int i = 0; i < num; i++)
       {
          for (int j = 0; j < numClasses_; j++)
          {
             if (boxes_[i].Class == j)
             {
                rosBoxes_[j].push_back(boxes_[i]);
                rosBoxCounter_[j]++;
                std::cout << classLabels_[boxes_[i].Class] << " (" << boxes_->prob << ")" << std::endl;
             }
          }
       }

       // send message that an object has been detected
       std_msgs::Int8 msg;
       msg.data = 1;
       objectPublisher_.publish(msg);

       for (int i = 0; i < numClasses_; i++)
       {
         if (rosBoxCounter_[i] > 0) drawBoxes(input_frame, rosBoxes_[i],
                                                 rosBoxCounter_[i], rosBoxColors_[i], classLabels_[i]);
       }
       bboxesPublisher_.publish(bboxesResults_);
       bboxesResults_.bboxes.clear();
      }
      else
      {
        std_msgs::Int8 msg;
        msg.data = 0;
        objectPublisher_.publish(msg);
      }

      for (int i = 0; i < numClasses_; i++)
      {
         rosBoxes_[i].clear();
         rosBoxCounter_[i] = 0;
      }

      cv::imshow(opencvWindow_, input_frame);
      cv::waitKey(3);
   }

   void cameraCallback(const sensor_msgs::ImageConstPtr& msg)
   {
      std::cout << "usb image received" << std::endl;

      cv_bridge::CvImagePtr cam_image;

      try
      {
        cam_image = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
      }
      catch (cv_bridge::Exception& e)
      {
         ROS_ERROR("cv_bridge exception: %s", e.what());
         return;
      }

      if (cam_image)
      {
         camImageCopy_ = cam_image->image.clone();

         if (frameCount_ == 0)
         {
            runYolo(cam_image->image);
            frameWidth_ = cam_image->image.size().width;
            frameHeight_ = cam_image->image.size().height;
         }
         //frameCount_++;
         if (frameCount_ == 1) frameCount_ = 0;
      }
      return;
   }
};

int main(int argc, char** argv)
{
   ros::init(argc, argv, "ros_interface");

   std::string param;
   float thresh;
   ros::param::get("/darknet_rsl/object_threshold", thresh);
   ros::param::get("/darknet_rsl/camera_topic_name", cameraTopicName_);
   ros::param::get("/darknet_rsl/weights_path", param);
   char *weights = new char[param.length() + 1];
   strcpy(weights, param.c_str());
   ros::param::get("/darknet_rsl/cfg_path", param);
   char *cfg = new char[param.length() + 1];
   strcpy(cfg, param.c_str());

   load_network(cfg, weights, thresh);

   YoloObjectDetector yod;
   ros::spin();
   return 0;
}
