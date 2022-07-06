#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <pcl/features/normal_3d.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/io/pcd_io.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <string>

using namespace std;

// char directory[200] = "/media/rambo/ssd2/Szilard/nyu_v2_filter/comparison/ddd/ddd_n10/";
char directory[200] = "/home/rambo/pvcnn/data/own/02958343/depth/";
int cnt = 0;
int max_nr = 150;
int min_points = 0;

void depth2pcd()
{
    char file_depthin[100];
    // sprintf(file_depthin, "%sdepth_ae/%05d_ddd.png", directory, cnt);
    sprintf(file_depthin, "%shplane_%d.png", directory, cnt);
    printf("Processing file - %s - with number %05d\n", file_depthin, cnt);
    // char file_depthout[100];
    // sprintf(file_depthout, "%sadepth/%05d_depth.png", directory, cnt);
    // char file_irout[100];
    // sprintf(file_irout, "%sair/%05d_ir.png", directory, cnt);
    // char file_d2iout[100];
    // sprintf(file_d2iout, "%sadepth2ir/%05d_depth2ir.png", directory, cnt);

    cv::Mat depth = cv::imread(file_depthin, CV_LOAD_IMAGE_UNCHANGED);
    // vector<cv::Mat> channels(3);
    // cv::split(depth3, channels);
    // cv::Mat depth = channels[1];
    // cv::Mat outi = channels[2];
    // cv::imwrite(file_depthout, depth);
    // cv::imwrite(file_irout, outi);
    // cv::imwrite(file_d2iout, depth2ir);

    double K[9] = {460.58518931365654, 0.0, 334.0805877590529, 0.0, 460.2679961517268, 169.80766383231037, 0.0, 0.0, 1.0}; // pico zense
    // double K[9] = {460.585, 0.0, 334.081, 0.0, 460.268, 169.808, 0.0, 0.0, 1.0}; // pico zense
    // double K[9] = {582.62448167737955, 0.0, 313.04475870804731, 0.0, 582.69103270988637, 238.44389626620386, 0.0, 0.0, 1.0}; // nyu_v2_dataset
    // double K[9] = {582.624, 0.0, 313.045, 0.0, 582.691, 238.444, 0.0, 0.0, 1.0}; // nyu_v2_dataset
    double fx = K[0];
    double fy = K[4];
    double x0 = K[2];
    double y0 = K[5];

    pcl::PointCloud<pcl::PointXYZ> cloud_msg;
    pcl::PointXYZ p;
    for (int i = 0; i < depth.rows; i++)
    {
        for (int j = 0; j < depth.cols; j++)
        {
            int index = i * depth.cols + j;
            float d = depth.at<uint16_t>(i, j) / 1000.0;

            if (d == 0.0)
            {
                continue;
            }
            float x_over_z = (j - x0) / fx;
            float y_over_z = (i - y0) / fy;
            p.z = d;
            p.x = x_over_z * p.z;
            p.y = y_over_z * p.z;

            cloud_msg.points.push_back(p);
        }
    }
    cloud_msg.width = cloud_msg.points.size();
    cout << "Number of points:" << cloud_msg.width << endl;
    if (cloud_msg.width > min_points)
    {
        cloud_msg.height = 1;
        cloud_msg.points.resize(cloud_msg.width * cloud_msg.height);
        cloud_msg.is_dense = false;
        char file_pcd[100];
        sprintf(file_pcd, "%shplane_%d.pcd", directory, cnt);
        pcl::io::savePCDFile(file_pcd, cloud_msg, true);
        cout << "[*] Conversion finished!" << endl;
    }
    else
    {
        cout << "[*] Conversion failed - too few points!" << endl;
    }

    cnt=cnt+1;
}

int main(int argc, char **argv)
{

    // char files[100];
    // sprintf(files, "%snoisydepth/filelist.txt", directory);
    // ifstream myfile(files);
    // if (myfile.is_open())
    // {
    //     string line;
    //     while (getline(myfile, line))
    //     {
    //         int n = line.length();
    //         char file[n + 1];
    //         strcpy(file, line.c_str());
    //         depth2pcd(file);
    //     }
    //     myfile.close();
    // }
    while (cnt <= max_nr)
    {
        depth2pcd();
    }
    printf("The data is processed. End the application.\n");
    return 0;
}
