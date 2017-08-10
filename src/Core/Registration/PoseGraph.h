// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2017 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#pragma once

#include <vector>
#include <memory>

#include <IO/ClassIO/IJsonConvertible.h>
#include <Core/Utility/Eigen.h>

namespace three {

class PoseGraphNode : public IJsonConvertible
{
public:
	PoseGraphNode(Eigen::Matrix4d pose = Eigen::Matrix4d::Identity()) :
			pose_(pose) {};
	~PoseGraphNode();

public:
	bool ConvertToJsonValue(Json::Value &value) const override;
	bool ConvertFromJsonValue(const Json::Value &value) override;

public:
	Eigen::Matrix4d pose_;
};

class PoseGraphEdge : public IJsonConvertible
{
public:
	PoseGraphEdge(
			int target_node_id = -1, int source_node_id = -1,
			Eigen::Matrix4d transformation = Eigen::Matrix4d::Identity(),
			Eigen::Matrix6d information = Eigen::Matrix6d::Identity(),
			bool uncertain = false,
			double confidence = 1.0) :
			target_node_id_(target_node_id),
			source_node_id_(source_node_id),
			transformation_(transformation),
			information_(information),
			uncertain_(uncertain),
			confidence_(confidence) {};
	~PoseGraphEdge();

public:
	bool ConvertToJsonValue(Json::Value &value) const override;
	bool ConvertFromJsonValue(const Json::Value &value) override;

public:
	int target_node_id_;
	int source_node_id_;
	Eigen::Matrix4d transformation_;
	Eigen::Matrix6d information_;
	/// odometry edge has uncertain == false
	/// loop closure edges has uncertain == true
	bool uncertain_;
	/// if uncertain_ is true, it has confidence bounded in [0,1].
	/// 1 means reliable, and 0 means unreliable edge.
	/// This correspondence to line process value in [Choi et al 2015]
	/// See core/registration/globaloptimization.h for more details.
	double confidence_;
};

class PoseGraph : public IJsonConvertible
{
public:
	PoseGraph();
	~PoseGraph() override;

public:
	bool ConvertToJsonValue(Json::Value &value) const override;
	bool ConvertFromJsonValue(const Json::Value &value) override;

public:
	std::vector<PoseGraphNode> nodes_;
	std::vector<PoseGraphEdge> edges_;
};

/// Factory function to create a PoseGraph from a file
/// (PinholeCameraTrajectoryFactory.cpp)
/// Return an empty PinholeCameraTrajectory if fail to read the file.
std::shared_ptr<PoseGraph> CreatePoseGraphFromFile(
		const std::string &filename);

}	// namespace three
