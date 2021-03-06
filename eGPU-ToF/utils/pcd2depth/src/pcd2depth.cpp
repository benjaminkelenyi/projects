// Include opencv2
#include <opencv2/core.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <pcl/common/eigen.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/io/pcd_io.h>
#include <pcl/ModelCoefficients.h>
#include <pcl/point_types.h>
#include <pcl/range_image/range_image_planar.h>
#include <pcl/sample_consensus/method_types.h>
#include <pcl/sample_consensus/model_types.h>
#include <pcl/segmentation/sac_segmentation.h>

// #include "matplotlibcpp.h"

using namespace cv;
// namespace plt = matplotlibcpp;

using namespace std;

int main(int argc, const char**argv)
{
    // calibration parameters
    // double K[9] = {385.8655956930966, 0.0, 342.3593021849471,
    //                0.0, 387.13463636528166, 233.38372018194542,
    //                0.0, 0.0, 1.0};

    // EEPROM parameters
    double K[9] = {460.58518931365654, 0.0, 334.0805877590529, 0.0, 460.2679961517268, 169.80766383231037, 0.0, 0.0, 1.0}; // pico zense
    // double K[9] = {582.62448167737955, 0.0, 313.04475870804731, 0.0, 582.69103270988637, 238.44389626620386, 0.0, 0.0, 1.0}; // nyu_v2_dataset
    double fx = K[0];
    double fy = K[4];
    double x0 = K[2];
    double y0 = K[5];
    int height_ = 480;
    int width_ = 640;
    int pixel_pos_x, pixel_pos_y;
    double z, u, v;
    cv::Mat cv_image;
    std::vector<Point2d> imagePoints;

    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    std::string filename = "";
    // std::cin >> filename;

    struct dirent *entry = nullptr;
    DIR *dp = nullptr;

    dp = opendir(argc > 1 ? argv[1] : "/home/rambo/pvcnn/data/own/02958343/pcd/");
    if (dp != nullptr) 
    {
        while ((entry = readdir(dp)))
        {
            std::string path = "/home/rambo/pvcnn/data/own/02958343/pcd/";
            std::string filename = path + entry->d_name;

            if (filename.find(".pcd") != std::string::npos)
            {

                // printf("Processing: %s\n", filename);
                if (pcl::io::loadPCDFile<pcl::PointXYZ>(filename, *cloud) == -1) //* load the file
                {
                    PCL_ERROR("Couldn't read file \n");
                    return (-1);
                }

                std::cout << "PointCloud has: " << cloud->size() << " data points." << std::endl;
                //cv_image = Mat(height_, width_, CV_32FC1, 0.0); //Scalar(std::numeric_limits<float>::max()));
                cv::Mat output = cv::Mat::zeros(height_, width_, CV_16UC1);
                for (int i = 0; i < cloud->points.size(); i++)
                {
                    bool nan = false;
                    if (isnan(cloud->points[i].x) || isnan(cloud->points[i].y) || isnan(cloud->points[i].z)) nan=true;
                    if (isinf(cloud->points[i].x) || isinf(cloud->points[i].y) || isinf(cloud->points[i].z)) nan=true;
                    if (cloud->points[i].z<=(fy/1000.0)) nan=true;
                    if (!nan)
                    {
                        z = cloud->points[i].z * 1000.0;
                        u = (cloud->points[i].x * 1000.0 * fx) / z;
                        v = (cloud->points[i].y * 1000.0 * fy) / z;
                        pixel_pos_x = (int)(u + x0);
                        pixel_pos_y = (int)(v + y0);

                        if (pixel_pos_x < 0)
                        {
                            pixel_pos_x = -pixel_pos_x;
                        }
                        if (pixel_pos_x > (width_ - 1))
                        {
                            pixel_pos_x = width_ - 1;
                        }

                        if (pixel_pos_y < 0)
                        {
                            pixel_pos_y = -pixel_pos_y;
                        }
                        if (pixel_pos_y > (height_ - 1))
                        {
                            pixel_pos_y = height_ - 1;
                        }
                        // std::cout<<"pixels="<<pixel_pos_x<<","<<pixel_pos_y<<","<<z<<std::endl;
                        output.at<uint16_t>(pixel_pos_y, pixel_pos_x) = z;
                        // output.at<Vec3b>(pixel_pos_y, pixel_pos_x)[1] = z;
                        // output.at<Vec3b>(pixel_pos_y, pixel_pos_x)[2] = z;
                    }
                }

                // std::cout << "Params: " << maxx << ", " << maxy << ", " << maxz << std::endl;
                //output.convertTo(output, CV_16UC3);

                // imshow("Display depth from point cloud", cv_image);
                // waitKey(3);
                filename = filename.substr(0, filename.size() - 3);
                imwrite(filename + "png", output);
            }
        }
    }

    // printf("Image is saved to: %s.png\n",filename);
    //sensor_msgs::ImagePtr output_image = cv_bridge::CvImage(std_msgs::Header(), "16UC1", cv_image).toImageMsg();
    //pub_.publish(output_image);
}
