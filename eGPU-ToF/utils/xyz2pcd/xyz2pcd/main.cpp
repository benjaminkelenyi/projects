#include <iostream>
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <string>
#include <iostream>
#include <experimental/filesystem>
#include <stdio.h>
#include <dirent.h>
using namespace std;


/* ------------------------------ xyz2pcd ---------------------------------- */

// int main(int argc, char *argv[])
// {
//     vector<string> result;
//     pcl::PointCloud<pcl::PointXYZRGB> cloud;
//     pcl::PointXYZRGB p;
//     string filename = "";

//     // cout << "[*] This tool will convert your .xyz with rgb information into .pcd v7 file." << endl;
//     // cout << "[*] Write the .xyz filename, e.g. test.xyz : " << endl;
//     // cin >> filename;

//     // ifstream xyz_file(filename);
//     // //Check that the input file has being successfully opened
//     // if (!(xyz_file.is_open()))
//     // {
//     //     cout << "[x] Cannot open file!" << endl;
//     // }

//     struct dirent *entry = nullptr;
//     DIR *dp = nullptr;

//     dp = opendir(argc > 1 ? argv[1] : "/home/rambo/pvcnn/data/own/02958343/");
//     if (dp != nullptr) 
//     {
//         while ((entry = readdir(dp)))
//         {
//             std::string path = "/home/rambo/pvcnn/data/own/02958343/";
//             std::string filename = path + entry->d_name;

//             if (filename.find(".txt") != std::string::npos)
//             {
//                 string content = "";
//                 int i = 0;

//                 ifstream xyz_file(filename);

//                 while (getline(xyz_file, content))
//                 {
//                     boost::split(result, content, [](char c) { return c == ' '; });
//                     //        cout << "x: " << result[0] << " y: " << result[1] << " z: " << result[2] << endl;
//                     p.x = stof(result[0]);
//                     p.y = stof(result[1]);
//                     p.z = stof(result[2]);
//                     // p.r = stof(result[3]);
//                     // p.g = stof(result[4]);
//                     // p.b = stof(result[5]);
//                     cloud.points.push_back(p);
//                     i++;
//                 }

//                 cloud.width = cloud.points.size();
//                 cout << "Number of points:" << cloud.width << endl;
//                 cloud.height = 1;
//                 cloud.points.resize(cloud.width * cloud.height);
//                 cloud.is_dense = false;
//                 xyz_file.close();
//                 filename = filename.substr(0, filename.size() - 3);
//                 printf ("Name: %s \n", filename.c_str());
//                 pcl::io::savePCDFileASCII(filename + "pcd", cloud);
//                 cout << "[*] Conversion finished!" << endl;
//                 cloud.clear();
//             }
//         }
//     }
// }


/* ------------------------------ pcd2xyz ---------------------------------- */
int main(int argc, const char**argv)
{
    struct dirent *entry = nullptr;
    DIR *dp = nullptr;

    dp = opendir(argc > 1 ? argv[1] : "/home/rambo/pvcnn/data/own/02691156/depth/pcd/");
    if (dp != nullptr) 
    {
        while ((entry = readdir(dp)))
        {
            std::string path = "/home/rambo/pvcnn/data/own/02691156/depth/pcd/";
            std::string filename = path + entry->d_name;

            if (filename.find(".pcd") != std::string::npos)
            {
                printf ("Name: %s \n", filename.c_str());

                pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);

                // cout << "[*] Write the .pcd filename " << endl;
                // std::cin >> filename;
                pcl::PCDReader reader;
                reader.read(filename.c_str(), *cloud);
                filename = filename.substr(0, filename.size() - 3);
                ofstream myfile;
                myfile.open(filename + "xyz");
                for (int i = 0; i < cloud->size(); i++)
                {
                    myfile << cloud->points[i].x << " " << cloud->points[i].y << " " << cloud->points[i].z << "\n";
                }
                //myfile << "Writing this to a file.\n";
                myfile.close();
            }
        }
    }
    closedir(dp);    

    cout << "[*] Conversion finished!" << endl;
    return 0;
}

// int main(int argc, const char**argv) {
//     struct dirent *entry = nullptr;
//     DIR *dp = nullptr;

//     dp = opendir(argc > 1 ? argv[1] : "/home/rambo/pvcnn/data/own/02691156/");
//     if (dp != nullptr) 
//     {
//         while ((entry = readdir(dp)))
//         {   
//             std::string path = "/home/rambo/pvcnn/data/own/02691156/";
//             std::string test = path + entry->d_name;
//             printf ("%s \n", test.c_str());
//             // strcat("/home/rambo/pvcnn/data/own/cylinder/pcd/" + entry->d_name)
//             // printf ("%s\n", "/home/rambo/pvcnn/data/own/cylinder/pcd/" + entry->d_name);
//         }
//     }

//     closedir(dp);
//     return 0;
// }