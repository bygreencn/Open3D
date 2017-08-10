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

#include "PointCloud.h"

#include <Eigen/Dense>
#include <Core/Utility/Console.h>
#include <Core/Geometry/Image.h>
#include <Core/Camera/PinholeCameraIntrinsic.h>
#include <IO/ClassIO/PointCloudIO.h>

namespace three{

std::shared_ptr<PointCloud> CreatePointCloudFromFile(
		const std::string &filename)
{
	auto pointcloud = std::make_shared<PointCloud>();
	ReadPointCloud(filename, *pointcloud);
	return pointcloud;
}

std::shared_ptr<PointCloud> CreatePointCloudFromDepthImage(
		const Image &depth, const PinholeCameraIntrinsic &intrinsic,
		const Eigen::Matrix4d &extrinsic/* = Eigen::Matrix4d::Identity()*/, 
		double depth_scale/* = 1000.0*/)
{
	auto pointcloud = std::make_shared<PointCloud>();
	if (depth.num_of_channels_ != 1 || depth.bytes_per_channel_ != 2) {
		PrintDebug("[CreatePointCloudFromDepthImage] Unsupported image format.\n");
		return pointcloud;
	}
	Eigen::Matrix4d camera_pose = extrinsic.inverse();
	auto focal_length = intrinsic.GetFocalLength();
	auto principal_point = intrinsic.GetPrincipalPoint();
	for (int i = 0; i < depth.height_; i++) {
		uint16_t *p = (uint16_t *)(depth.data_.data() + 
				i * depth.BytesPerLine());
		for (int j = 0; j < depth.width_; j++, p++) {
			if (*p > 0) {
				double z = (double)(*p) / depth_scale;
				double x = (j - principal_point.first) * z /
						focal_length.first;
				double y = (i - principal_point.second) * z /
						focal_length.second;
				Eigen::Vector4d point = camera_pose * 
						Eigen::Vector4d(x, y, z, 1.0);
				pointcloud->points_.push_back(point.block<3, 1>(0, 0));
			}
		}
	}
	return pointcloud;
}

std::shared_ptr<PointCloud> CreatePointCloudFromRGBDImage(
		const Image &depth, const Image &color,
		const PinholeCameraIntrinsic &intrinsic,
		const Eigen::Matrix4d &extrinsic/* = Eigen::Matrix4d::Identity()*/, 
		double depth_scale/* = 1000.0*/)
{
	auto pointcloud = std::make_shared<PointCloud>();
	if (depth.num_of_channels_ != 1 || depth.bytes_per_channel_ != 2 ||
			color.num_of_channels_ != 3 || color.bytes_per_channel_ != 1) {
		PrintDebug("[CreatePointCloudFromRGBDImage] Unsupported image format.\n");
		return pointcloud;
	}
	Eigen::Matrix4d camera_pose = extrinsic.inverse();
	auto focal_length = intrinsic.GetFocalLength();
	auto principal_point = intrinsic.GetPrincipalPoint();
	for (int i = 0; i < depth.height_; i++) {
		uint16_t *p = (uint16_t *)(depth.data_.data() + 
				i * depth.BytesPerLine());
		uint8_t *pc = (uint8_t *)(color.data_.data() + 
				i * color.BytesPerLine());
		for (int j = 0; j < depth.width_; j++, p++, pc += 3) {
			if (*p > 0) {
				double z = (double)(*p) / depth_scale;
				double x = (j - principal_point.first) * z /
						focal_length.first;
				double y = (i - principal_point.second) * z /
						focal_length.second;
				Eigen::Vector4d point = camera_pose * 
						Eigen::Vector4d(x, y, z, 1.0);
				pointcloud->points_.push_back(point.block<3, 1>(0, 0));
				pointcloud->colors_.push_back(Eigen::Vector3d(
						pc[0], pc[1], pc[2]) / 255.0);
			}
		}
	}
	return pointcloud;
}

}	// namespace three
