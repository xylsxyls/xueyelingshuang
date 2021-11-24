#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class Camera;

class CameraGrabTask : public CTask
{
public:
	CameraGrabTask();

public:
	void DoTask();

	void StopTask();

	void setParam(Camera* camera);

private:
	Camera* m_camera;
	std::atomic<bool> m_exit;
};