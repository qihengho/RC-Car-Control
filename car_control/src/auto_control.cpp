/* Reads velocity (linear and angular) commands from move_base node and then publishes it to topic,
 * that the Arduino servo controller will subscribe to. 
 * 
 * Copyright (C) 2008, Morgan Quigley and Willow Garage, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Stanford University or Willow Garage, Inc. nor the 
 *     names of its contributors may be used to endorse or promote products 
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "ros/ros.h"
#include "std_msgs/UInt32.h"
#include "car_control/joystick.h"
#include "geometry_msgs/Twist.h"
#include <sensor_msgs/Joy.h>
#include <sstream>

uint32_t joynavigation = 90100;
uint32_t autonavigation = 90100;
std_msgs::UInt32 navigation;
// time in seconds since last joy message received before automatic stop
const double noMessageThreshold = 2;
double timeSinceLastMessage = noMessageThreshold;
// make sure this is the same as in the Arduino
const double runFrequency = 60;
int steeringPosition = 90;
int ThrottlePosition = 100;

double maxVelocity = 0.5;
double maxSteering = 0.005;
bool flag = false;
int throttle = 100;
int steering = 90;

/**********************************************************************************
 If no signal is received from the XBox controller after a while due to a disconnect 
 or error, "steering 90 throttle 110" is sent to the Arduino to instruct it to turn
 the car straight and brake. 
 **********************************************************************************/
void stopCallback()
{
  navigation.data = 90100;
}

void joystickCallback(const sensor_msgs::Joy::ConstPtr& msg) 
{
  int button = msg->buttons[2];
  if (button == 1){
	  flag = !flag;
  }
  
  // for normal forward operation. the two integers must be declared in this scope. 
  int steeringJoystickPosition = 94 + (msg->axes[2] * 20);
  int throttleLeverPosition = 100 - (msg->axes[1] * 15);

  // extreme low speed on Y button hold for mapping purposes
  if (msg->axes[5] == 1)
  {
    throttleLeverPosition = 93;
  }

  // this makes the brakes more responsive and enables a reverse function
  if (msg->axes[1] < -0.03)
  {
    throttleLeverPosition = 100 - (msg->axes[1] * 24);
  }

  // emergency brake
  if (msg->axes[5] == -1)
  {
    throttleLeverPosition = 150;
  }


  // first three digits steering, last three digits throttle
  joynavigation = (steeringJoystickPosition * 1000) + throttleLeverPosition;

  timeSinceLastMessage = 0;

}

/**********************************************************************************
 The XBox controller outputs a (class?) of button, joystick, and lever position data.
 In here, we call it msg, and extract the data corresponding to the positions of the 
 steering joystick and acceleration lever. 
 This callback only executes upon receipt of message from joystick, and does not 
 directly send a message to the Arduino. It only modifies the message sent. 
 **********************************************************************************/
void navigationCallback(const geometry_msgs::Twist::ConstPtr& msg) 
{
  
  // first three digits steering, last three digits throttle
  //navigation.data = (steeringJoystickPosition * 1000) + throttleLeverPosition;

  if (flag == false){
	  if (msg->linear.x > 0){
		  throttle = 105;
	  }
	  else {
		  if (msg->linear.x < 0){
		  throttle = 95;
	  }
  else {
	  throttle = 100;
	 }
 }
	 
      if (msg->angular.z > 0){
		  steering = 100;
	  }
	  else {
		  if (msg->angular.z < 0){
		  steering = 80;
	  }
  else {
	  steering = 90;
  }
}
	  autonavigation = (steering * 1000) + throttle;
	  timeSinceLastMessage = 0;
  
}
}


int main(int argc, char **argv)
{
  /**********************************************************************************
   The ros::init() function needs to see argc and argv so that it can perform
   any ROS arguments and name remapping that were provided at the command line.
   For programmatic remappings you can use a different version of init() which takes
   remappings directly, but for most command-line programs, passing argc and argv is
   the easiest way to do it.  The third argument to init() is the name of the node.
   
   You must call one of the versions of ros::init() before using any other
   part of the ROS system.
   **********************************************************************************/
  ros::init(argc, argv, "car_control");

  /**********************************************************************************
   NodeHandle is the main access point to communications with the ROS system.
   The first NodeHandle constructed will fully initialize this node, and the last
   NodeHandle destructed will close down the node.
   **********************************************************************************/
  ros::NodeHandle n;

  /**********************************************************************************
   Subscribes to topic joy, which the joystick publishes button and lever data to. 
   On receipt of data, executes function joystickCallback, which converts joystick
   data to a UInt8 which will be sent to the Arduino. 
   **********************************************************************************/
  ros::Subscriber sub = n.subscribe("twisty", 1000, navigationCallback);
  ros::Subscriber suby = n.subscribe("joy", 1000, joystickCallback);

  /**********************************************************************************
   The advertise() function is how you tell ROS that you want to publish on a given 
   topic name. This invokes a call to the ROS master node, which keeps a registry of 
   who is publishing and who is subscribing. After this advertise() call is made, 
   the master node will notify anyone who is trying to subscribe to this topic name,
   and they will in turn negotiate a peer-to-peer connection with this node.  
   advertise() returns a Publisher object which allows you to publish messages on 
   that topic through a call to publish(). Once all copies of the returned Publisher
   object are destroyed, the topic will be automatically unadvertised.
   
   The second parameter to advertise() is the size of the message queue used for
   publishing messages.  If messages are published more quickly than we can send them,
   the number here specifies how many messages to buffer up before throwing some away.
   **********************************************************************************/
  ros::Publisher pub = n.advertise<std_msgs::UInt32>("arduino", 1000);

  ros::Rate loop_rate(runFrequency);

  while (ros::ok())
  {
    // DEBUG
    ROS_INFO("%u", navigation.data);


    timeSinceLastMessage = timeSinceLastMessage + (1 / runFrequency);
    
    if (timeSinceLastMessage > noMessageThreshold) 
      stopCallback();

    /*******************************************************************************
     The publish() function is how you send messages. The parameter is the message
     object. The type of this object must agree with the type given as a template
     parameter to the advertise<>() call, as was done in the constructor above.
     *******************************************************************************/
    if (flag){
		navigation.data = joynavigation;
		pub.publish(navigation);
	}
	else{
		navigation.data = autonavigation;
		pub.publish(navigation);
	}
	
    ros::spinOnce();

    loop_rate.sleep();
  }

  return 0;
}
