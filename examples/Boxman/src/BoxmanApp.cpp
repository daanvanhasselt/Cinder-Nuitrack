#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"
#include "cinder/CameraUi.h"
#include "cinder/Utilities.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#include "Cinder-Nuitrack.h"

using namespace tdv::nuitrack;

class Bone {
public:
	Bone(JointType _from, JointType _to, vec3 _direction) {
		from = _from;
		to = _to;
		direction = _direction;
	}

	JointType from;
	JointType to;
	vec3 direction;
};

class BoxmanApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;

protected:
	vector<Skeleton> skeletons;
	void drawSkeleton(Skeleton &s, ColorA color);
	void drawSkeletonInfo(Skeleton &s, ColorA color, int w, int h);
	void drawJoint(Joint &j, ColorA color);
	void drawBones(vector<Joint> joints, ColorA color);

	vector<ColorA> skeletonColors = {
		ColorA(1, 0, 0, 1),
		ColorA(0, 1, 0, 1),
		ColorA(0, 0, 1, 1),
		ColorA(1, 1, 0, 1),
		ColorA(1, 0, 1, 1),
		ColorA(0, 1, 1, 1),
		ColorA(1, 1, 1, 1),
		ColorA(1, 0.5, 0.5, 1),
		ColorA(0.5, 1, 0.5, 1),
		ColorA(0.5, 0.5, 1, 1)
	};

	cinui::TrackerRef tracker;
	vec3 floorPoint;
	vec3 floorNormal;
	gl::BatchRef floorPlaneBatch;

	void setupGrid();
	void drawGrid();
	gl::BatchRef horizontalPlaneBatch;
	gl::BatchRef verticalPlaneBatch;

	void setupCam();
	CameraPersp viewCam;
	CameraUi camUi;

	gl::Texture2dRef rgbTex;
	gl::Texture2dRef depthTex;
};

void BoxmanApp::setup()
{
	tracker = cinui::Tracker::create();
	tracker->init();
	tracker->setIssuesCallback([this](IssuesData::Ptr data) {
		auto issue = data->getIssue<Issue>();
		if (issue) {
			CI_LOG_I("Issue detected! " << issue->getName() << " [" << issue->getId() << "]");
		}
	});

	tracker->setRGBCallback([this](RGBFrame::Ptr data) {
		int rows = data->getRows();
		int cols = data->getCols();
		const Color3 *colorData = data->getData();
		uint8_t *colorDataPtr = (uint8_t *)colorData;
		SurfaceRef s = Surface::create(colorDataPtr, cols, rows, cols * 3, SurfaceChannelOrder::BGR);
		rgbTex = gl::Texture2d::create(*s);
	});

	tracker->setDepthCallback([this](DepthFrame::Ptr data) {
		int rows = data->getRows();
		int cols = data->getCols();
		const uint16_t *colorData = data->getData();
		uint8_t *colorDataPtr = (uint8_t *)colorData;
		Channel8uRef s = Channel8u::create(cols, rows, cols * 2, 2, colorDataPtr);
		depthTex = gl::Texture2d::create(*s);
	});


	tracker->setSkeletonCallback([this](SkeletonData::Ptr data) {
		skeletons = data->getSkeletons();
	});

	tracker->setUserCallback([this](UserFrame::Ptr data) {
		floorPoint = cinui::Tracker::fromVector3(data->getFloor()) / vec3(1000);
		floorNormal = cinui::Tracker::fromVector3(data->getFloorNormal());

		//// for reorienting the scene
		//vec3 up = vec3(0, 1, 0);
		//vec3 newY = glm::normalize(floorNormal);
		//vec3 newZ = glm::normalize(glm::cross(newY, up));
		//vec3 newX = glm::normalize(glm::cross(newY, newZ));
		//floorTransform = mat3(newX, newY, newZ);
		//// now multiply the body points by this matrix to apply tilt correction
	});

	tracker->run();

	setupCam();
	setupGrid();
}

void BoxmanApp::setupCam() {
	viewCam.setEyePoint(vec3(0.0f, -0.25f, -2.0f));
	viewCam.setPerspective(60, getWindowWidth() / getWindowHeight(), 0.001, 1000);
	viewCam.lookAt(vec3(0));
	viewCam.setWorldUp(vec3(0, -1, 0));
	camUi = CameraUi(&viewCam);
	camUi.connect(getWindow());
}

void BoxmanApp::setupGrid() {
	auto color = gl::ShaderDef().color();
	auto shader = gl::getStockShader(color);

	// create 2 planes
	int subdivisions = 20;
	float size = 5;
	auto horizontalPlane = geom::WirePlane().subdivisions(ivec2(subdivisions, subdivisions)).size(vec2(size, size)).origin(vec3(0, -1, 0));
	auto verticalPlane = geom::WirePlane().axes(vec3(1, 0, 0), vec3(0, 1, 0)).subdivisions(ivec2(subdivisions, subdivisions)).size(vec2(size, size)).origin(vec3(0, -1 + size / 2, size / 2));
	auto white = geom::Constant(geom::COLOR, ColorA(1, 1, 1, 0.5));
	horizontalPlaneBatch = gl::Batch::create(horizontalPlane >> white, shader);
	verticalPlaneBatch = gl::Batch::create(verticalPlane >> white, shader);


	// setup floor plane
	auto floorPlane = geom::WirePlane().subdivisions(ivec2(subdivisions * 2, subdivisions * 2)).size(vec2(size * 2, size * 2)).origin(vec3(0, 0, 0)).normal(vec3(0, 1, 0));
	floorPlaneBatch = gl::Batch::create(floorPlane >> white, shader);
}


void BoxmanApp::update()
{
	getWindow()->setTitle("Boxman - fps: " + toString(getAverageFps()));
	tracker->poll();
}

void BoxmanApp::draw()
{
	gl::clear(Color(0, 0, 0));

	gl::pushMatrices();

	// turn on z-buffering
	gl::enableDepthRead();
	gl::enableDepthWrite();

	gl::enableAlphaBlending();

	gl::setMatrices(viewCam);

	// flip vertically
	gl::scale(1, -1, 1);

	drawGrid();

	static auto color = gl::ShaderDef().color();
	static auto shader = gl::getStockShader(color);
	gl::ScopedGlslProg prog(shader);

	// draw skeletons
	for (int i = 0; i < skeletons.size(); i++) {
		auto s = skeletons[i];
		drawSkeleton(s, skeletonColors[i]);
	}
	gl::popMatrices();


	// turn off z-buffering for flat texture drawing
	gl::pushMatrices();
	gl::disableDepthRead();
	gl::disableDepthWrite();

	gl::color(1, 1, 1);

	// color stream
	if (rgbTex) {
		gl::draw(rgbTex, Rectf(0, 0, 320, 240));
	}
	// depth stream,
	if (depthTex) {
		gl::translate(320 + 10, 0, 0);
		gl::draw(depthTex, Rectf(0, 0, 320, 240));
	}
	gl::popMatrices();

	// skeleton info
	gl::pushMatrices();
	int w = 300;
	int h = 100;
	for (int i = 0; i < skeletons.size(); i++) {
		auto s = skeletons[i];
		drawSkeletonInfo(s, skeletonColors[i], w, h);
		gl::translate(w, 0);
	}
	gl::popMatrices();
}

void BoxmanApp::drawGrid() {
	//horizontalPlaneBatch->draw();
	verticalPlaneBatch->draw();

	static auto color = gl::ShaderDef().color();
	static auto shader = gl::getStockShader(color);
	static auto constantColor = geom::Constant(geom::COLOR, ColorA(0.5, 1, 0.5, 0.3));
	auto floorPlane = geom::WirePlane().subdivisions(ivec2(20, 20)).size(vec2(10, 10)).origin(floorPoint).normal(floorNormal);
	floorPlaneBatch = gl::Batch::create(floorPlane >> constantColor, shader);

	gl::pushMatrices();
	floorPlaneBatch->draw();
	gl::popMatrices();
}

void BoxmanApp::drawSkeleton(Skeleton &s, ColorA color) {
	auto joints = s.joints;
	for (auto j : joints) {
		drawJoint(j, color);
	}

	drawBones(joints, color);
}

void BoxmanApp::drawSkeletonInfo(Skeleton &s, ColorA color, int w, int h) {
	auto user = tracker->getUser(s.id);
	if (user.id != s.id) {
		return;
	}

	int id = user.id;
	vec3 centerOfMass = vec3(user.real.x, user.real.y, user.real.z) / vec3(1000);
	vec3 centerOfMassNormalized = vec3(user.proj.x, user.proj.y, 0);
	float occlusion = user.occlusion;
	float avgConfidence = 0;
	float minConfidence = 1;
	float maxConfidence = 0;
	for (auto j : s.joints) {
		if (j.type == JOINT_NONE) {
			continue;
		}

		avgConfidence += j.confidence;
		if (j.confidence > maxConfidence) {
			maxConfidence = j.confidence;
		}
		if (j.confidence < minConfidence) {
			minConfidence = j.confidence;
		}
	}
	avgConfidence /= s.joints.size();

	Rectf bounds = Rectf(0, getWindowHeight() - h, w, getWindowHeight());

	gl::color(Color::black());
	gl::drawSolidRect(bounds);

	gl::color(color);
	gl::drawStrokedRect(bounds);

	// occlusion + confidence bars
	float barH = h / 2.0;
	gl::color(0.4, 0.2, 0.2);
	gl::drawSolidRect(Rectf(0, bounds.y1, w * occlusion, bounds.y1 + barH));

	gl::color(0.2, 0.5, 0.2);
	gl::drawSolidRect(Rectf(0, bounds.y1 + barH, w * maxConfidence, bounds.y1 + (barH * 2)));

	gl::color(0.2, 0.4, 0.2);
	gl::drawSolidRect(Rectf(0, bounds.y1 + barH, w * avgConfidence, bounds.y1 + (barH * 2)));

	gl::color(0.2, 0.3, 0.2);
	gl::drawSolidRect(Rectf(0, bounds.y1 + barH, w * minConfidence, bounds.y1 + (barH * 2)));

	// draw info strings
	gl::drawString("ID:\nCenter:\nCenter (p):\nOcclusion:\nConfidence (min,max,avg):", bounds.getUpperLeft() + vec2(10, 10));
	gl::drawString(toString(id) + "\n" + toString(centerOfMass) + "\n" + toString(centerOfMassNormalized) + "\n" + toString(occlusion) + "\n(" + toString(minConfidence) + ", " + toString(maxConfidence) + ", " + toString(avgConfidence) + ")",
		bounds.getUpperLeft() + vec2(w / 2.0, 10));
}

void BoxmanApp::drawJoint(Joint &j, ColorA color) {
	if (j.type == JOINT_NONE) {
		return;
	}

	if (j.confidence < 0.15) {
		return;
	}

	glm::quat quaternion = cinui::Tracker::fromOrientation(j.orient);

	// real pos is in mm, lets convert to m
	vec3 pos = cinui::Tracker::fromVector3(j.real) / vec3(1000);

	gl::pushMatrices();
	gl::translate(pos);
	gl::rotate(quaternion);

	gl::color(color);
	gl::drawCube(vec3(0), vec3(0.025));
	gl::drawCoordinateFrame(0.1, 0.05, 0.01);
	gl::popMatrices();
}

void BoxmanApp::drawBones(vector<Joint> joints, ColorA color) {
	if (joints.size() < 3) {
		return;
	}

	// rotation based on T pose
	// sdk reports rotations relative to T pose..
	vector<Bone> bones = {
		Bone(JOINT_WAIST, JOINT_TORSO, vec3(0, 1, 0)),
		Bone(JOINT_TORSO, JOINT_NECK, vec3(0, 1, 0)),
		Bone(JOINT_NECK, JOINT_HEAD, vec3(0, 1, 0)),

		Bone(JOINT_LEFT_COLLAR, JOINT_LEFT_SHOULDER, vec3(1, 0, 0)),
		Bone(JOINT_LEFT_SHOULDER, JOINT_LEFT_ELBOW, vec3(1, 0, 0)),
		Bone(JOINT_LEFT_ELBOW, JOINT_LEFT_WRIST, vec3(1, 0, 0)),
		Bone(JOINT_LEFT_WRIST, JOINT_LEFT_HAND, vec3(1, 0, 0)),

		Bone(JOINT_WAIST, JOINT_LEFT_HIP, vec3(1, 0, 0)),
		Bone(JOINT_LEFT_HIP, JOINT_LEFT_KNEE, vec3(0, -1, 0)),
		Bone(JOINT_LEFT_KNEE, JOINT_LEFT_ANKLE, vec3(0, -1, 0)),

		Bone(JOINT_RIGHT_COLLAR, JOINT_RIGHT_SHOULDER, vec3(-1, 0, 0)),
		Bone(JOINT_RIGHT_SHOULDER, JOINT_RIGHT_ELBOW, vec3(-1, 0, 0)),
		Bone(JOINT_RIGHT_ELBOW, JOINT_RIGHT_WRIST, vec3(-1, 0, 0)),
		Bone(JOINT_RIGHT_WRIST, JOINT_RIGHT_HAND, vec3(-1, 0, 0)),

		Bone(JOINT_WAIST, JOINT_RIGHT_HIP, vec3(-1, 0, 0)),
		Bone(JOINT_RIGHT_HIP, JOINT_RIGHT_KNEE, vec3(0, -1, 0)),
		Bone(JOINT_RIGHT_KNEE, JOINT_RIGHT_ANKLE, vec3(0, -1, 0)),

	};

	for (int i = 0; i < bones.size(); i++) {
		float size = 0.05;
		auto j1 = joints[bones[i].from];
		auto j2 = joints[bones[i].to];

		if (j1.confidence < 0.15 || j2.confidence < 0.15) {
			continue;
		}

		vec3 p1 = cinui::Tracker::fromVector3(j1.real) / vec3(1000);
		vec3 p2 = cinui::Tracker::fromVector3(j2.real) / vec3(1000);

		float distance = glm::distance(p1, p2);
		glm::quat quaternion = cinui::Tracker::fromOrientation(joints[bones[i].from].orient);

		gl::pushMatrices();

		// translate to from joint pos
		gl::translate(p1);

		// rotate by reported rotation
		gl::rotate(quaternion);

		gl::color(color);

		// draw bone in default t-pose direction
		gl::drawStrokedCube(vec3(distance / 2) * bones[i].direction, vec3(distance) * bones[i].direction + vec3(size));

		gl::popMatrices();
	}
}

CINDER_APP( BoxmanApp, RendererGl )
