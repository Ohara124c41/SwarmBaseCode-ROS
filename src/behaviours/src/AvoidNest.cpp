#include "AvoidNest.hpp"
#include <cmath>

AvoidNest::AvoidNest(std::string name, double cameraOffset, double cameraHeight) :
   _cameraOffset(cameraOffset),
   _cameraHeight(cameraHeight),
   _persist(false)   
{
   _tagSubscriber = _nh.subscribe(name + "/targets", 1, &AvoidNest::TagHandler, this);
}

double AvoidNest::HorizontalDistance(double x, double y, double z)
{
   double distanceFromCamera = hypot(hypot(x, y), z);
   return sqrt(distanceFromCamera*distanceFromCamera - _cameraHeight*_cameraHeight);
}

void AvoidNest::TagHandler(const apriltags_ros::AprilTagDetectionArray::ConstPtr& message)
{
   _tagsRight = 0;
   _tagsLeft  = 0;
   _tooClose = false;
   for(auto tag : message->detections)
   {
      if(tag.pose.pose.position.x + _cameraOffset > 0)
      {
         _tagsRight++;
      }
      else
      {
         _tagsLeft++;
      }
      auto tagPos = tag.pose.pose.position;
      if(HorizontalDistance(tagPos.x, tagPos.y, tagPos.z) <= 0.2)
      {
         _tooClose = true;
      }
   }
}

void AvoidNest::PersistenceCallback(const ros::TimerEvent& event)
{
   _persist = false;
}

Action AvoidNest::GetAction()
{
   Action reaction = _llAction;

   if(_persist)
   {
      return _savedAction;
   }

   if(_tooClose)
   {
      reaction.drive.left = -100;
      reaction.drive.right = -100;
      return reaction;
   }

   if(_tagsLeft > 0 || _tagsRight > 0)
   {
      if(_tagsLeft > _tagsRight)
      {
         reaction.drive.left = 100;
         reaction.drive.right = -100;
      }
      else
      {
         reaction.drive.left = -100;
         reaction.drive.right = 100;
      }

      _persist = true;
      _savedAction = reaction;
      _persistenceTimer = _nh.createTimer(ros::Duration(1.2), &AvoidNest::PersistenceCallback, this, true);
   }

   return reaction;
}