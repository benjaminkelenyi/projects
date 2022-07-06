#include <ros/ros.h>

#include <geometry_msgs/Twist.h>


using namespace ros;


int main(int argc, char **argv) {
  // Initialize the node
  init(argc, argv, "move");

  // Create a node handle
  NodeHandle node;

  // A publisher for the movement data
  Publisher pub = node.advertise<geometry_msgs::Twist>("cmd_vel", 10);

  // Drive forward at a given speed.  The robot points up the x-axis.
  geometry_msgs::Twist command;

  //TODO: change the values, and watch the results
  command.linear.x = 0.2;
  //command.linear.y = 0.0;
  //command.linear.z = 0.0;
  //command.angular.x = 0.0;
  //command.angular.y = 0.0;
  //command.angular.z = 0.0;

  // Loop at 10Hz, publishing movement commands until we shut down.
  Rate rate(10);
  while (ok()) {
    pub.publish(command);
    rate.sleep();
  }

  return 0;
}
