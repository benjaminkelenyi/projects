#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <sensor_msgs/LaserScan.h>

#define MINDISTANCE 0.5
#define SPEED 0.2

using namespace ros;

//this is a demo, so we can afford globals
geometry_msgs::Twist command;

void laserCallback(const sensor_msgs::LaserScanConstPtr lms)
{
        bool stuck = false;
        for (int i = 0; i < lms->ranges.size(); i++)
        {
            if (i > lms->ranges.size() * 0.3 && i < lms->ranges.size() * 0.7)
            {
                // printf("the range is: %f - %f \n",lms->ranges.size() * 0.4,lms->ranges.size() * 0.6);
                if (lms->ranges[i] < MINDISTANCE)
                {
                    // printf("Value [%d] is : %f \n",i,lms->ranges[i]);
                    stuck = true;
                }
            }
        }
        if (stuck)
        {
            // printf("I'm stuck\n");
            command.linear.x = 0.0;
            command.angular.z = 0.5;
        }
        else
        {
            // printf("I'm not stuck\n");
            command.linear.x = SPEED;
            command.angular.z = 0.0;
        }

}

int main(int argc, char **argv)
{
    // Initialize the node
    init(argc, argv, "wander");

    // Create a node handle
    NodeHandle node;

    // Subscribe to the laser scan
    // check the name of the laser scan topic with: rostopic list

    // TODO
    Subscriber sub = node.subscribe("/scan", 1000, laserCallback);

    // A publisher for the movement data
    Publisher pub = node.advertise<geometry_msgs::Twist>("/RosAria/cmd_vel", 10);

    // Drive forward at a given speed.  The robot points up the x-axis.

    command.linear.x = SPEED;
    command.linear.y = 0.0;
    command.linear.z = 0.0;
    command.angular.x = 0.0;
    command.angular.y = 0.0;
    command.angular.z = 0.0;

    // Loop at 10Hz, publishing movement commands until we shut down.
    Rate rate(10);
    while (ok())
    {
        pub.publish(command);
        rate.sleep();
        ros::spinOnce();
    }

    return 0;
}