//
//  Cinder-Nuitrack.cpp
//  
//
//  Created by Daan van Hasselt on 09/03/18.
//
//

#include "Cinder-Nuitrack.h"
#include "cinder/Log.h"

using namespace ci;
using namespace cinui;
using namespace tdv::nuitrack;

Tracker::Tracker() {
	onRGBHandle = 0;
	onDepthHandle = 0;
	onUserHandle = 0;
	onHandHandle = 0;
	onSkeletonHandle = 0;
}

Tracker::~Tracker() {
	Nuitrack::release();
}

TrackerRef cinui::Tracker::create()
{
	return TrackerRef(new Tracker());
}

void cinui::Tracker::init(string configPath) {
	try {
		Nuitrack::init(configPath);
	}
	catch (const tdv::nuitrack::Exception &e) {
		CI_LOG_E("Error initializing Nuitrack: " << e.what() << "\nPlease make sure a device is connected");
		return;
	}

	CI_LOG_D("Initialized Nuitrack succesfully");
}

void cinui::Tracker::run() {
	try {
		Nuitrack::run();
	}
	catch (const tdv::nuitrack::Exception &e) {
		CI_LOG_E("Error running Nuitrack: " << e.what());
	}
}

void cinui::Tracker::poll() {
	try
	{
		if (rgbTracker) Nuitrack::update(rgbTracker);
		if (depthTracker) Nuitrack::update(depthTracker);
		if (userTracker) Nuitrack::update(userTracker);
		if (handTracker) Nuitrack::update(handTracker);
		if (skeletonTracker) Nuitrack::update(skeletonTracker);
	}
	catch (const tdv::nuitrack::Exception& e)
	{
		CI_LOG_E("Error updating Nuitrack: " << e.what());
	}
}

User cinui::Tracker::getUser(int id)
{
	User invalidUser;
	invalidUser.id = id - 1000;

	if (!userTracker) {
		return invalidUser;
	}

	auto userFrame = userTracker->getUserFrame();
	if (!userFrame) {
		return invalidUser;
	}

	auto users = userFrame->getUsers();
	if (users.empty()) {
		return invalidUser;
	}

	for (auto u : users) {
		if (u.id == id) {
			return u;
		}
	}

	return invalidUser;
}

void cinui::Tracker::setIssuesCallback(function<void(IssuesData::Ptr)> onIssues)
{
	if (onIssuesHandle) {
		unbindIssuesCallback();
	}

	onIssuesHandle = Nuitrack::connectOnIssuesUpdate(onIssues);
}

void cinui::Tracker::unbindIssuesCallback()
{
	if (!onIssuesHandle) {
		return;
	}

	Nuitrack::disconnectOnIssuesUpdate(onIssuesHandle);
	onIssuesHandle = 0;
}

void cinui::Tracker::setRGBCallback(function<void(RGBFrame::Ptr)> onRGB)
{
	if (onRGBHandle) {
		unbindRGBCallback();
	}

	if (!rgbTracker) {
		rgbTracker = ColorSensor::create();
	}

	onRGBHandle = rgbTracker->connectOnNewFrame(onRGB);
}

void cinui::Tracker::unbindRGBCallback()
{
	if (rgbTracker) {
		rgbTracker->disconnectOnNewFrame(onRGBHandle);
	}

	onRGBHandle = 0;
}

void cinui::Tracker::setDepthCallback(function<void(DepthFrame::Ptr)> onDepth)
{
	if (onDepthHandle) {
		unbindDepthCallback();
	}

	if (!depthTracker) {
		depthTracker = DepthSensor::create();
	}

	onDepthHandle = depthTracker->connectOnNewFrame(onDepth);
}

void cinui::Tracker::unbindDepthCallback()
{
	if (depthTracker) {
		depthTracker->disconnectOnNewFrame(onDepthHandle);
	}

	onDepthHandle = 0;
}

void cinui::Tracker::setUserCallback(function<void(UserFrame::Ptr)> onUser)
{
	if (onUserHandle) {
		unbindUserCallback();
	}

	if (!userTracker) {
		userTracker = UserTracker::create();
	}

	onUserHandle = userTracker->connectOnUpdate(onUser);
}

void cinui::Tracker::unbindUserCallback()
{
	if (userTracker) {
		userTracker->disconnectOnUpdate(onUserHandle);
	}

	onUserHandle = 0;
}

void cinui::Tracker::setHandCallback(function<void(HandTrackerData::Ptr)> onHand)
{
	if (onHandHandle) {
		unbindHandCallback();
	}

	if (!handTracker) {
		handTracker = HandTracker::create();
	}

	onHandHandle = handTracker->connectOnUpdate(onHand);
}

void cinui::Tracker::unbindHandCallback()
{
	if (handTracker) {
		handTracker->disconnectOnUpdate(onHandHandle);
	}

	onHandHandle = 0;
}

void cinui::Tracker::setSkeletonCallback(function<void(SkeletonData::Ptr)> onSkeleton)
{
	if (onSkeletonHandle) {
		unbindSkeletonCallback();
	}

	if (!skeletonTracker) {
		skeletonTracker = SkeletonTracker::create();
	}

	onSkeletonHandle = skeletonTracker->connectOnUpdate(onSkeleton);
}

void cinui::Tracker::unbindSkeletonCallback()
{
	if (skeletonTracker) {
		skeletonTracker->disconnectOnUpdate(onSkeletonHandle);
	}

	onSkeletonHandle = 0;
}
